// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "io/squeue.hpp"

#include "io/atomics.hpp"

namespace boros::io {

    Submission::Submission(io_uring_sqe *s) noexcept : sqe(s) {
        sqe->flags       = 0;
        sqe->ioprio      = 0;
        sqe->rw_flags    = 0;
        sqe->buf_index   = 0;
        sqe->personality = 0;
        sqe->file_index  = 0;
        sqe->addr3       = 0;
        sqe->__pad2[0]   = 0;
    }

    void Submission::Prepare(std::uint8_t op, int fd, const void *addr, unsigned len, std::size_t off) {
        sqe->opcode = op;
        sqe->fd     = fd;
        sqe->off    = off;
        sqe->addr   = reinterpret_cast<std::uintptr_t>(addr);
        sqe->len    = len;
    }

    void SubmissionQueue::Map(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) {
        m_khead        = reinterpret_cast<unsigned *>(sq_mmap.ptr + p.sq_off.head);
        m_ktail        = reinterpret_cast<unsigned *>(sq_mmap.ptr + p.sq_off.tail);
        m_ring_mask    = *reinterpret_cast<unsigned *>(sq_mmap.ptr + p.sq_off.ring_mask);
        m_ring_entries = *reinterpret_cast<unsigned *>(sq_mmap.ptr + p.sq_off.ring_entries);
        m_kflags       = reinterpret_cast<unsigned *>(sq_mmap.ptr + p.sq_off.flags);
        m_entries      = reinterpret_cast<io_uring_sqe *>(sqe_mmap.ptr);

        if ((p.flags & IORING_SETUP_NO_SQARRAY) == 0) {
            // To keep things simple, map the slots directly to entries.
            volatile unsigned *array = reinterpret_cast<unsigned *>(sq_mmap.ptr + p.sq_off.array);
            for (unsigned i = 0; i < m_ring_entries; ++i) {
                array[i] = i;
            }
        }
    }

    bool SubmissionQueue::NeedCompletionQueueFlush() const {
        // Ordering: This is merely used as an informational function
        // in the event loop, so relaxed ordering is fine.
        auto flags = impl::AtomicLoad(m_kflags, std::memory_order_relaxed);
        return (flags & IORING_SQ_CQ_OVERFLOW) != 0;
    }

    // Ordering: The following functions do not require synchronization
    // on their khead and ktail accesses, because the kernel only reads
    // these fields in response to a io_uring_enter syscall.

    unsigned SubmissionQueue::GetSize() const {
        return *m_ktail - *m_khead;
    }

    bool SubmissionQueue::HasCapacityFor(unsigned num) const {
        return (m_ring_entries - this->GetSize()) >= num;
    }

    Submission SubmissionQueue::Push() {
        auto *sqe = &m_entries[*m_ktail & m_ring_mask];
        ++*m_ktail;
        return Submission{sqe};
    }

}  // namespace boros::io
