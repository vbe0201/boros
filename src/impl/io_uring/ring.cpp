// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "io_uring/ring.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "macros.hpp"

namespace boros {

    namespace {

        ALWAYS_INLINE auto SetupRing(unsigned entries, io_uring_params *p) noexcept -> int {
            int res = static_cast<int>(syscall(__NR_io_uring_setup, entries, p));
            if (res < 0) [[unlikely]] {
                return -errno;
            }
            return res;
        }

        ALWAYS_INLINE auto RegisterRing(int fd, unsigned int op, const void *arg, unsigned nargs) noexcept -> int {
            int res = static_cast<int>(syscall(__NR_io_uring_register, fd, op, arg, nargs));
            if (res < 0) [[unlikely]] {
                return -errno;
            }
            return res;
        }

        ALWAYS_INLINE auto EnterRing(int fd, unsigned nsubmit, unsigned want, unsigned flags) noexcept -> int {
            int res = static_cast<int>(syscall(__NR_io_uring_enter, fd, nsubmit, want, flags, nullptr, 0));
            if (res < 0) [[unlikely]] {
                return -errno;
            }
            return res;
        }

    }

    IoRing::~IoRing() noexcept {
        this->Finalize();
    }

    auto IoRing::Initialize(unsigned int entries, io_uring_params &p) noexcept -> int {
        // Create the io_uring instance in the kernel.
        // We try to configure it without SQARRAY first...
        p.flags |= IORING_SETUP_NO_SQARRAY;
        int fd = SetupRing(entries, &p);
        if (fd < 0 && fd != -EINVAL) [[unlikely]] {
            return fd;
        }

        // ...and if that didn't work, try again with it.
        p.flags &= ~IORING_SETUP_NO_SQARRAY;
        fd = SetupRing(entries, &p);
        if (fd < 0) [[unlikely]] {
            return fd;
        }

        // Now try to map in the created ring queues.
        if (int res = this->InitializeWithFile(fd, p); res != 0) [[unlikely]] {
            close(fd);
            return res;
        }

        return 0;
    }

    auto IoRing::InitializeWithFile(int fd, io_uring_params &p) noexcept -> int {
        Mmap scq_mmap;
        Mmap sqe_mmap;

        // If the kernel does not support a single mmap for both
        // queues, the user is on an unsupported kernel.
        if ((p.features & IORING_FEAT_SINGLE_MMAP) != 0) [[unlikely]] {
            return -EOPNOTSUPP;
        }

        // Determine queue sizes for the ring.
        std::size_t scq_len = std::max(
            p.sq_off.array + p.sq_entries * sizeof(unsigned),
            p.cq_off.cqes + p.cq_entries * sizeof(io_uring_cqe)
        );
        std::size_t sqe_len = p.sq_entries * sizeof(io_uring_sqe);

        // Map the submission queue and the completion queue in.
        if (auto res = scq_mmap.Map(fd, IORING_OFF_SQ_RING, scq_len); res != 0) [[unlikely]] {
            return res;
        }

        // Map the submission queue elements in.
        if (auto res = sqe_mmap.Map(fd, IORING_OFF_SQES, sqe_len); res != 0) [[unlikely]] {
            return res;
        }

        // Store io_uring instance data in this object.
        m_submission_queue.Map(p, scq_mmap, sqe_mmap);
        m_completion_queue.Map(p, scq_mmap);
        m_ring_fd  = fd;
        m_enter_fd = fd;
        m_flags    = p.flags;
        m_features = p.features;
        m_scq_mmap = std::move(scq_mmap);
        m_sqe_mmap = std::move(sqe_mmap);

        return 0;
    }

    auto IoRing::Finalize() noexcept -> void {
        // Unmap the ring queues.
        m_scq_mmap.Unmap();
        m_sqe_mmap.Unmap();

        // If we have a direct descriptor, free its slot.
        if (m_registered) {
            this->UnregisterRingFd();
        }

        // Dispose of the ring file descriptor, if we have one.
        if (m_ring_fd != -1) {
            close(m_ring_fd);
            m_ring_fd = -1;
        }
    }

    auto IoRing::RegisterRaw(unsigned opcode, const void *arg, unsigned nargs) const noexcept -> int {
        int fd;

        // If registered and supported, use the registered fd.
        // If not registered, fallback to the ring fd.
        if (m_registered && (m_features & IORING_FEAT_REG_REG_RING) != 0) {
            // Kernel 6.3+ supports using a direct descriptor for register
            // calls. So do exactly that to save some overhead.
            opcode |= IORING_REGISTER_USE_REGISTERED_RING;
            fd = m_enter_fd;
        } else if (m_ring_fd != -1) {
            // We cannot use or do not have a direct descriptor
            // but we can fallback to the ring file descriptor.
            fd = m_ring_fd;
        } else [[unlikely]] {
            // Ring fd was closed, but the kernel is too old to
            // support using the direct descriptor here. So we
            // can only error.
            return -EOPNOTSUPP;
        }

        return RegisterRing(fd, opcode, arg, nargs);
    }

    auto IoRing::RegisterFilesSparse(unsigned nfiles) const noexcept -> int {
        io_uring_rsrc_register reg{};
        reg.nr    = nfiles;
        reg.flags = IORING_RSRC_REGISTER_SPARSE;

        return this->RegisterRaw(IORING_REGISTER_FILES2, &reg, sizeof(reg));
    }

    auto IoRing::UnregisterFiles() const noexcept -> int {
        return this->RegisterRaw(IORING_UNREGISTER_FILES, nullptr, 0);
    }

    auto IoRing::RegisterProbe(io_uring_probe* probe) const noexcept -> int {
		return this->RegisterRaw(IORING_REGISTER_PROBE, probe, 256);
	}

	auto IoRing::Enable() const noexcept -> int {
		return this->RegisterRaw(IORING_REGISTER_ENABLE_RINGS, nullptr, 0);
	}

	auto IoRing::RegisterRingFd() noexcept -> int {
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

	auto IoRing::UnregisterRingFd() noexcept -> int {
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

	auto IoRing::CloseRingFd() noexcept -> int {
    	if ((m_features & IORING_FEAT_REG_REG_RING) == 0) [[unlikely]] {
    		return -EOPNOTSUPP;
    	}
    	if (!m_registered) [[unlikely]] {
			return -EINVAL;
    	}
		if (m_ring_fd == -1) [[unlikely]] {
			return -EBADF;
		}

    	close(m_ring_fd);
    	m_ring_fd = -1;

    	return 1;
	}

	auto IoRing::RegisterBufferRing(io_uring_buf_reg *reg) const noexcept -> int {
	    return this->RegisterRaw(IORING_REGISTER_PBUF_RING, reg, 1);
    }

	auto IoRing::UnregisterBufferRing(int bgid) const noexcept -> int {
	    io_uring_buf_reg reg{};
    	reg.bgid = bgid;

    	return this->RegisterRaw(IORING_UNREGISTER_PBUF_RING, &reg, 1);
    }

    auto IoRing::SubmitAndWait(unsigned want) const noexcept -> int {
       	unsigned num_submit  = m_submission_queue.Synchronize();
       	unsigned enter_flags = 0;

       	// When the Completion Queue overflows with IORING_FEAT_NODROP enabled, the
       	// kernel buffers these overflown events but doesn't automatically flush them
       	// to the queue when space becomes available. This mandates an io_uring_enter
       	// call, even with IORING_SETUP_SQPOLL enabled.
       	bool cq_needs_enter = want > 0 || m_submission_queue.NeedCompletionQueueFlush();

       	if ((m_flags & IORING_SETUP_SQPOLL) != 0) {
       		// Ordering: Sequential consistency is required to ensure our write to the
       		// ktail is observed by the kernel before reading the flags below.
       		std::atomic_thread_fence(std::memory_order_seq_cst);

       		if (m_submission_queue.NeedWakeup()) [[unlikely]] {
       			enter_flags |= IORING_ENTER_SQ_WAKEUP;
       		} else if (!cq_needs_enter) {
       			return static_cast<int>(num_submit);
       		}
       	}

       	if (cq_needs_enter) {
       		enter_flags |= IORING_ENTER_GETEVENTS;
       	}

       	if (m_registered) {
       		enter_flags |= IORING_ENTER_REGISTERED_RING;
       	}

       	return EnterRing(m_enter_fd, num_submit, want, enter_flags);
    }

}
