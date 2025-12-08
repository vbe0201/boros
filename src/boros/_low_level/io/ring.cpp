// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "io/ring.hpp"

#include <algorithm>
#include <bit>
#include <cerrno>

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace boros::io {

    namespace {

        unsigned CalculateCqSize(unsigned sq, unsigned cq) {
            constexpr unsigned MaxValue = std::numeric_limits<unsigned>::max();

            // An explicit completion queue size must meet two criteria:
            //   1. It must be greater than sq.
            //   2. It must be a power of two.
            //
            // So this is our attempt at wrangling whatever cq value we
            // get into one that gets accepted by io_uring_setup.

            // Check if it makes sense to set up an explicit cq size.
            if (cq == 0 || sq == MaxValue || sq == cq) {
                return 0;
            }

            // Pick the bigger of both values as our baseline. This is
            // guaranteed to be at least 1 since we checked cq already.
            cq = std::max(sq, cq);

            // Calculate one less than the next power of two.
            if (cq == 1) {
                cq = 0;
            }
            cq = MaxValue >> std::countl_zero(cq - 1);

            // If we're not about to overflow the integer, add 1 to get
            // the next power of two. Otherwise, leave the value as-is
            // to not violate rule 1.
            if (cq != MaxValue) {
                ++cq;
            }

            return cq;
        }

        int SetupRing(unsigned entries, io_uring_params *p) {
            int res = static_cast<int>(syscall(__NR_io_uring_setup, entries, p));
            if (res < 0) {
                return -errno;
            }

            return res;
        }

        int RegisterRing(int fd, unsigned op, const void *arg, unsigned nargs) {
            int res = static_cast<int>(syscall(__NR_io_uring_register, fd, op, arg, nargs));
            if (res < 0) {
                return -errno;
            }

            return res;
        }

        int EnterRing(int fd, unsigned nsubmit, unsigned want, unsigned flags, void *arg, std::size_t size) {
            int res = static_cast<int>(syscall(__NR_io_uring_enter, fd, nsubmit, want, flags, arg, size));
            if (res < 0) {
                return -errno;
            }

            return res;
        }

    }  // namespace

    Ring::~Ring() {
        this->Finalize();
    }

    int Ring::Setup(unsigned entries, io_uring_params &p) {
        // Try creating the ring without SQARRAY first for kernel 6.6+.
        p.flags |= IORING_SETUP_NO_SQARRAY;
        int fd = SetupRing(entries, &p);
        if (fd < 0 && fd != -EINVAL) {
            return fd;
        }

        // That didn't work, so try again without the flag for older kernels.
        p.flags &= ~IORING_SETUP_NO_SQARRAY;
        fd = SetupRing(entries, &p);
        if (fd < 0) {
            return fd;
        }

        // Map in the created ring queues.
        if (int res = this->SetupWithFile(fd, p); res != 0) {
            close(fd);
            return res;
        }

        return 0;
    }

    int Ring::SetupWithFile(int fd, io_uring_params &p) {
        Mmap scq_map;
        Mmap sqe_map;

        // Support for a single mmap for both queues is required,
        // otherwise the user is running an unsupported kernel.
        if ((p.features & IORING_FEAT_SINGLE_MMAP) == 0) {
            return -EOPNOTSUPP;
        }

        // Determine queue sizes for the ring.
        std::size_t scq_len = std::max(p.sq_off.array + p.sq_entries * sizeof(unsigned),
                                       p.cq_off.cqes + p.cq_entries * sizeof(io_uring_cqe));
        std::size_t sqe_len = p.sq_entries * sizeof(io_uring_sqe);

        // Map the queues in.
        if (int res = scq_map.Map(fd, IORING_OFF_SQ_RING, scq_len); res != 0) {
            return res;
        }

        // Map the submission entries in.
        if (int res = sqe_map.Map(fd, IORING_OFF_SQES, sqe_len); res != 0) {
            return res;
        }

        m_submission_queue.Map(p, scq_map, sqe_map);
        m_completion_queue.Map(p, scq_map);
        m_ring_fd  = fd;
        m_enter_fd = fd;
        m_flags    = p.flags;
        m_features = p.features;
        m_scq_map  = std::move(scq_map);
        m_sqe_map  = std::move(sqe_map);
        return 0;
    }

    int Ring::Initialize(unsigned sq_entries, unsigned cq_entries) {
        io_uring_params params{};

        // Configure a completion queue size, if given.
        cq_entries = CalculateCqSize(sq_entries, cq_entries);
        if (cq_entries != 0) {
            params.flags |= IORING_SETUP_CQSIZE;
            params.cq_entries = cq_entries;
        }

        // Clamp the submission queue size at the max number of entries.
        // This reduces configuration errors with invalid parameters.
        params.flags |= IORING_SETUP_CLAMP;
        // Create the ring in disabled state by default. Allows for some
        // additional setup before submissions are allowed.
        params.flags |= IORING_SETUP_R_DISABLED;
        // Submit all requests to the kernel even when one of them fails
        // inline. We do not care about that since we still receive a
        // completion event with the error and can then handle it.
        params.flags |= IORING_SETUP_SUBMIT_ALL;
        // Decouple async event reaping and retrying from regular system
        // calls. If this isn't set, then io_uring uses normal task_work
        // for this and we could end up running that way too often. This
        // flag defers task_work to when the event loop enters the kernel
        // anyway to wait for new events.
        params.flags |= IORING_SETUP_DEFER_TASKRUN;
        // Inform the kernel that only a single thread submits to the ring.
        // This enables internal performance optimizations since our ring
        // is only designed for single-threaded usage anyway.
        params.flags |= IORING_SETUP_SINGLE_ISSUER;

        return this->Setup(sq_entries, params);
    }

    void Ring::Finalize() {
        m_scq_map.Unmap();
        m_sqe_map.Unmap();

        if (m_registered) {
            this->UnregisterRingFd();
        }

        if (m_ring_fd != -1) {
            close(m_ring_fd);
            m_ring_fd = -1;
        }
    }

    int Ring::Register(unsigned opcode, const void *arg, unsigned nargs) const {
        int fd;

        if (m_registered && (m_features & IORING_FEAT_REG_REG_RING) != 0) {
            // Kernel 6.3+ supports direct descriptors for register.
            // Use that to save some overhead on the system call.
            opcode |= IORING_REGISTER_USE_REGISTERED_RING;
            fd = m_enter_fd;
        } else if (m_ring_fd != -1) {
            // Fall back to the regular ring file descriptor.
            fd = m_ring_fd;
        } else [[unlikely]] {
            // Ring fd was closed, but the kernel is too old to
            // support direct descriptors here.
            return -EOPNOTSUPP;
        }

        return RegisterRing(fd, opcode, arg, nargs);
    }

    int Ring::Enable() const {
        return this->Register(IORING_REGISTER_ENABLE_RINGS, nullptr, 0);
    }

    int Ring::RegisterRingFd() {
        io_uring_rsrc_update upd{};
        upd.data   = m_ring_fd;
        upd.offset = -1U;

        if (m_registered) {
            return -EEXIST;
        }

        int res = this->Register(IORING_REGISTER_RING_FDS, &upd, 1);
        if (res == 1) {
            m_enter_fd   = static_cast<int>(upd.offset);
            m_registered = true;
        }

        return res;
    }

    int Ring::UnregisterRingFd() {
        io_uring_rsrc_update upd{};
        upd.offset = m_enter_fd;

        if (m_ring_fd == -1 || !m_registered) {
            return -EINVAL;
        }

        int res = this->Register(IORING_UNREGISTER_RING_FDS, &upd, 1);
        if (res == 1) {
            m_enter_fd   = m_ring_fd;
            m_registered = false;
        }

        return res;
    }

    int Ring::Enter(unsigned want, __kernel_timespec *ts) const {
        unsigned nsubmit     = m_submission_queue.GetSize();
        unsigned enter_flags = 0;
        void *enter_arg      = nullptr;
        std::size_t arg_size = 0;

        // Determine if we need to enter the kernel and wait for events.
        // This is the case when we are waiting for some completions or
        // when the CQ overflowed and must be flushed.
        bool need_getevents = want > 0 || m_submission_queue.NeedCompletionQueueFlush();

        // If we have a timeout, prepare the argument.
        io_uring_getevents_arg arg;
        if (ts != nullptr) {
            arg       = {0, 0, 0, reinterpret_cast<std::uintptr_t>(ts)};
            enter_arg = &arg;
            arg_size  = sizeof(arg);
            enter_flags |= IORING_ENTER_EXT_ARG;
        }

        // If we have submissions or need to wait for events, enter the
        // kernel with appropriate flags.
        if (nsubmit > 0 || need_getevents) {
            if (need_getevents) {
                enter_flags |= IORING_ENTER_GETEVENTS;
            }

            if (m_registered) {
                enter_flags |= IORING_ENTER_REGISTERED_RING;
            }

            return EnterRing(m_enter_fd, nsubmit, want, enter_flags, enter_arg, arg_size);
        }

        return 0;
    }

    int Ring::Submit() const {
        return this->Enter(0, nullptr);
    }

    int Ring::Wait(unsigned want, std::optional<std::chrono::milliseconds> timeout) const {
        constexpr std::chrono::seconds MinSecs{0};
        constexpr std::chrono::nanoseconds MinNanos{0};
        constexpr std::chrono::nanoseconds MaxNanos{999'999'999};

        // If we have a timeout, convert it into a __kernel_timespec
        // as a deadline. Otherwise, wait indefinitely.
        if (timeout) {
            auto secs = std::max(std::chrono::duration_cast<std::chrono::seconds>(*timeout), MinSecs);
            auto nanos =
                std::clamp(std::chrono::duration_cast<std::chrono::nanoseconds>(*timeout - secs), MinNanos, MaxNanos);
            __kernel_timespec ts{secs.count(), nanos.count()};

            return this->Enter(want, &ts);
        } else {
            return this->Enter(want, nullptr);
        }
    }

}  // namespace boros::io
