#include "ring.hpp"

#include <algorithm>

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace boros::impl {

    auto Ring::RegisterRaw(unsigned int opcode, const void* arg, unsigned int num_args) const noexcept -> int {
        if (m_registered) {
            opcode |= IORING_REGISTER_USE_REGISTERED_RING;
        }

        int res = static_cast<int>(syscall(__NR_io_uring_register, m_enter_fd, opcode, arg, num_args));
        if (res < 0) [[unlikely]] {
            return -errno;
        }

        return 0;
    }

    auto Ring::Create(unsigned entries, io_uring_params& p) noexcept -> int {
        // Allocate a file descriptor for the io_uring we're going to manage.
        // This can also be a direct descriptor, depending on the params.
        int fd = static_cast<int>(syscall(__NR_io_uring_setup, entries, &p));
        if (fd < 0) [[unlikely]] {
            return -errno;
        }

        // Try to create the ring instance. If we fail, dispose of the file.
        if (int res = this->CreateWithFile(fd, p); res != 0) [[unlikely]] {
            close(fd);
            return res;
        }

        return 0;
    }

    auto Ring::CreateWithFile(int fd, io_uring_params& p) noexcept -> int {
        Mmap sq_mmap;
        Mmap cq_mmap;
        Mmap sqe_mmap;

        std::size_t sq_len  = p.sq_off.array + p.sq_entries * sizeof(unsigned);
        std::size_t cq_len  = p.cq_off.cqes + p.cq_entries * sizeof(io_uring_cqe);
        std::size_t sqe_len = p.sq_entries * sizeof(io_uring_sqe);

        // Map the Submission Queue entries into memory.
        if (auto res = sqe_mmap.Create(fd, IORING_OFF_SQES, sqe_len); res != 0) [[unlikely]] {
            return res;
        }

        if ((p.features & IORING_FEAT_SINGLE_MMAP) != 0) {
            // Submission Queue and Completion Queue will be merged into a single mapping,
            // so pick the bigger of both queue sizes as the size of that mapping.
            if (cq_len > sq_len) {
                sq_len = cq_len;
            }
        } else {
            // Map the Completion Queue into memory (disjoint from Submission Queue).
            if (auto res = cq_mmap.Create(fd, IORING_OFF_CQ_RING, cq_len); res != 0) [[unlikely]] {
                return res;
            }
        }

        // Map the Submission Queue into memory. If IORING_FEAT_SINGLE_MMAP is supported,
        // this mapping will also accommodate for the Completion Queue.
        if (auto res = sq_mmap.Create(fd, IORING_OFF_SQ_RING, sq_len); res != 0) [[unlikely]] {
            return res;
        }

        m_submission_queue.Map(p, sq_mmap, sqe_mmap);
        m_completion_queue.Map(p, cq_mmap.IsMapped() ? cq_mmap : sq_mmap);
        m_ring_fd  = fd;
        m_enter_fd = fd;
        m_flags    = p.flags;
        m_features = p.features;
        m_sq_mmap  = std::move(sq_mmap);
        m_sqe_mmap = std::move(sqe_mmap);
        m_cq_mmap  = std::move(cq_mmap);
        return 0;
    }

    Ring::~Ring() noexcept {
        // Destroy the rings. From here, we cannot access the queues anymore. This is
        // done explicitly to ensure the mappings are gone before the handle is closed.
        m_sq_mmap.Destroy();
        m_sqe_mmap.Destroy();
        m_cq_mmap.Destroy();

        // If we have a direct descriptor, free its slot.
        if (m_registered) {
	        this->UnregisterRingFd();
        }

        // Dispose of the ring file descriptor, if we have one.
        if (m_ring_fd != -1) {
            close(m_ring_fd);
        }
    }

    auto Ring::RegisterFilesSparse(unsigned num_files) const noexcept -> int {
        io_uring_rsrc_register reg{};
        reg.nr    = num_files;
        reg.flags = IORING_RSRC_REGISTER_SPARSE;

        return this->RegisterRaw(IORING_REGISTER_FILES2, &reg, sizeof(reg));
    }

    auto Ring::UnregisterFiles() const noexcept -> int {
        return this->RegisterRaw(IORING_UNREGISTER_FILES, nullptr, 0);
    }

    auto Ring::RegisterEventFd(int fd) const noexcept -> int {
        return this->RegisterRaw(IORING_REGISTER_EVENTFD, &fd, 1);
    }

    auto Ring::RegisterEventFdAsync(int fd) const noexcept -> int {
        return this->RegisterRaw(IORING_REGISTER_EVENTFD_ASYNC, &fd, 1);
    }

    auto Ring::UnregisterEventFd() const noexcept -> int {
        return this->RegisterRaw(IORING_UNREGISTER_EVENTFD, nullptr, 0);
    }

	auto Ring::RegisterProbe(io_uring_probe* probe) const noexcept -> int {
		return this->RegisterRaw(IORING_REGISTER_PROBE, probe, 256);
	}

	auto Ring::Enable() const noexcept -> int {
		return this->RegisterRaw(IORING_REGISTER_ENABLE_RINGS, nullptr, 0);
	}

	auto Ring::RegisterRingFd() noexcept -> int {
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

	auto Ring::UnregisterRingFd() noexcept -> int {
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

	auto Ring::CloseRingFd() noexcept -> int {
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

	auto Ring::RegisterNapi(io_uring_napi* napi) const noexcept -> int {
		return this->RegisterRaw(IORING_REGISTER_NAPI, napi, 1);
	}

	auto Ring::UnregisterNapi(io_uring_napi* napi) const noexcept -> int {
		return this->RegisterRaw(IORING_UNREGISTER_NAPI, napi, 1);
	}

	auto Ring::RegisterBufRing(io_uring_buf_reg *reg) const noexcept -> int {
	    return this->RegisterRaw(IORING_REGISTER_PBUF_RING, reg, 1);
    }

	auto Ring::UnregisterBufRing(int bgid) const noexcept -> int {
	    io_uring_buf_reg reg{};
    	reg.bgid = bgid;

    	return this->RegisterRaw(IORING_UNREGISTER_PBUF_RING, &reg, 1);
    }

	auto Ring::GetBufRingHead(int buf_group, std::uint16_t *head) const noexcept -> int {
	    io_uring_buf_status status{};
    	status.buf_group = buf_group;

    	int res = this->RegisterRaw(IORING_REGISTER_PBUF_STATUS, &status, 1);
    	if (res != 0) [[unlikely]] {
    		return res;
    	}

    	*head = status.head;
    	return 0;
    }

}
