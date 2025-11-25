// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "ring.hpp"

#include <algorithm>
#include <bit>
#include <cerrno>

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "macros.hpp"

namespace boros {

    namespace {

        ALWAYS_INLINE auto SetupRing(unsigned entries, io_uring_params *p) -> int {
            int res = static_cast<int>(syscall(__NR_io_uring_setup, entries, p));
            if (res < 0) [[unlikely]] {
                return -errno;
            }

            return res;
        }

        ALWAYS_INLINE auto RegisterRing(int fd, unsigned op, const void *arg, unsigned nargs) -> int {
            int res = static_cast<int>(syscall(__NR_io_uring_register, fd, op, arg, nargs));
            if (res < 0) [[unlikely]] {
                return -errno;
            }

            return res;
        }

        ALWAYS_INLINE auto EnterRing(int fd, unsigned nsubmit, unsigned want, unsigned flags, void *arg, std::size_t size) -> int {
            int res = static_cast<int>(syscall(__NR_io_uring_enter, fd, nsubmit, want, flags, arg, size));
            if (res < 0) [[unlikely]] {
                return -errno;
            }

            return res;
        }

    }

    IoRing::~IoRing() {
        this->Finalize();
    }

    auto IoRing::Setup(unsigned entries, io_uring_params &p) -> int {
        // Try creating the ring without SQARRAY first for kernel 6.6+.
        p.flags |= IORING_SETUP_NO_SQARRAY;
        int fd = SetupRing(entries, &p);
        if (fd < 0 && fd != -EINVAL) [[unlikely]] {
            return fd;
        }

        // That didn't work, so try again without the flag for older kernels.
        p.flags &= ~IORING_SETUP_NO_SQARRAY;
        fd = SetupRing(entries, &p);
        if (fd < 0) [[unlikely]] {
            return fd;
        }

        // Map in the created ring queues.
        if (int res = this->SetupWithFile(fd, p); res != 0) [[unlikely]] {
            close(fd);
            return res;
        }

        return 0;
    }

    auto IoRing::SetupWithFile(int fd, io_uring_params &p) -> int {
        Mmap scq_map;
        Mmap sqe_map;

        // If the kernel does not support a single mmap for both queues,
        // the user is running on an unsupported kernel.
        if ((p.features & IORING_FEAT_SINGLE_MMAP) == 0) [[unlikely]] {
            return -EOPNOTSUPP;
        }

        // Determine queue sizes for the ring.
        std::size_t scq_len = std::max(
            p.sq_off.array + p.sq_entries * sizeof(unsigned),
            p.cq_off.cqes + p.cq_entries * sizeof(io_uring_cqe)
        );
        std::size_t sqe_len = p.sq_entries * sizeof(io_uring_sqe);

        // Map the submission and completion queues in.
        if (int res = scq_map.Map(fd, IORING_OFF_SQ_RING, scq_len); res != 0) [[unlikely]] {
            return res;
        }

        // Map the submission queue slots in.
        if (int res = sqe_map.Map(fd, IORING_OFF_SQES, sqe_len); res != 0) [[unlikely]] {
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

    auto IoRing::Initialize(unsigned sq_entries, unsigned cq_entries) -> int {
        io_uring_params params{};

        // Configure an explicit completion queue size, if given.
        if (cq_entries != 0) {
            params.flags |= IORING_SETUP_CQSIZE;

            // The size of cq_entries must be greater than sq_entries
            // and a power of two. This is an approximation.
            cq_entries = std::max(sq_entries, cq_entries);
            params.cq_entries = 1U << (BITSIZEOF(cq_entries) - std::countl_zero(cq_entries));
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

    auto IoRing::Finalize() -> void {
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

    auto IoRing::RegisterRaw(unsigned opcode, const void *arg, unsigned nargs) const -> int {
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

    auto IoRing::RegisterFilesSparse(unsigned nfiles) const -> int {
        io_uring_rsrc_register reg{};
        reg.nr    = nfiles;
        reg.flags = IORING_RSRC_REGISTER_SPARSE;

        return this->RegisterRaw(IORING_REGISTER_FILES2, &reg, sizeof(reg));
    }

    auto IoRing::UnregisterFiles() const -> int {
        return this->RegisterRaw(IORING_UNREGISTER_FILES, nullptr, 0);
    }

    auto IoRing::Enable() const -> int {
        return this->RegisterRaw(IORING_REGISTER_ENABLE_RINGS, nullptr, 0);
    }

    auto IoRing::RegisterRingFd() -> int {
        io_uring_rsrc_update upd{};
        upd.data   = m_ring_fd;
        upd.offset = -1U;

        if (m_registered) [[unlikely]] {
            return -EEXIST;
        }

        int res = this->RegisterRaw(IORING_REGISTER_RING_FDS, &upd, 1);
        if (res == 1) [[likely]] {
            m_enter_fd   = static_cast<int>(upd.offset);
            m_registered = true;
        }

        return res;
    }

    auto IoRing::UnregisterRingFd() -> int {
        io_uring_rsrc_update upd{};
        upd.offset = m_enter_fd;

        if (m_ring_fd == -1 || !m_registered) [[unlikely]] {
            return -EINVAL;
        }

        int res = this->RegisterRaw(IORING_UNREGISTER_RING_FDS, &upd, 1);
        if (res == 1) [[likely]] {
            m_enter_fd   = m_ring_fd;
            m_registered = false;
        }

        return res;
    }

    auto IoRing::Enter(unsigned want, __kernel_timespec *ts) const -> int {
        unsigned num_submit  = m_submission_queue.Synchronize();
        unsigned enter_flags = 0;
        void *enter_arg      = nullptr;
        std::size_t arg_size = 0;

        // If the CQ overflowed, we must enter the kernel to flush the
        // events to the queue. This doesn't happen automatically.
        bool cq_needs_enter = want > 0 || m_submission_queue.NeedCompletionQueueFlush();

        // Prepare a timeout argument, if we have one.
        io_uring_getevents_arg arg;
        if (ts != nullptr) {
            arg = {0, 0, 0, reinterpret_cast<std::uintptr_t>(ts)};
            enter_flags |= IORING_ENTER_EXT_ARG;
            enter_arg = &arg;
            arg_size  = sizeof(arg);
        }

        if (num_submit > 0 || cq_needs_enter) {
            if (cq_needs_enter) {
                enter_flags |= IORING_ENTER_GETEVENTS;
            }

            if (m_registered) {
                enter_flags |= IORING_ENTER_REGISTERED_RING;
            }

            return EnterRing(m_enter_fd, num_submit, want, enter_flags, enter_arg, arg_size);
        }

        return 0;
    }

    auto IoRing::Submit() const -> int {
        return this->SubmitAndWait(0);
    }

    auto IoRing::SubmitAndWait(unsigned want, std::optional<std::chrono::milliseconds> timeout) const -> int {
        constexpr std::chrono::seconds MinSecs{0};
        constexpr std::chrono::nanoseconds MinNanos{0};
        constexpr std::chrono::nanoseconds MaxNanos{999'999'999};

        if (timeout) {
            auto secs  = std::max(std::chrono::duration_cast<std::chrono::seconds>(*timeout), MinSecs);
            auto nanos = std::clamp(std::chrono::duration_cast<std::chrono::nanoseconds>(*timeout - secs), MinNanos, MaxNanos);
            __kernel_timespec ts{secs.count(), nanos.count()};

            return this->Enter(want, &ts);
        } else {
            return this->Enter(want, nullptr);
        }
    }

}
