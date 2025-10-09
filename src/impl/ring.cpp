#include "ring.hpp"

#include <algorithm>

#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace boros::impl {

    auto Ring::Create(unsigned entries, io_uring_params& p) noexcept -> int {
        // Allocate a file descriptor for the io_uring we're going to manage.
        // This can also be a direct descriptor, depending on the params.
        int fd = static_cast<int>(syscall(__NR_io_uring_setup, entries, &p));
        if (fd < 0) [[unlikely]] {
            return errno;
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

        // Dispose of the ring file descriptor, if we have one.
        if (m_ring_fd != -1) {
            close(m_ring_fd);
        }
    }

}
