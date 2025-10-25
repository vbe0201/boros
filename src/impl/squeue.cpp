// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "squeue.hpp"
#include "utils.hpp"

namespace boros {

    namespace {

        ALWAYS_INLINE auto ClearSqe(io_uring_sqe *sqe) noexcept -> void {
            sqe->flags       = 0;
            sqe->ioprio      = 0;
            sqe->rw_flags    = 0;
            sqe->buf_index   = 0;
            sqe->personality = 0;
            sqe->file_index  = 0;
            sqe->addr3       = 0;
            sqe->__pad2[0]   = 0;
        }

    }

    auto SubmissionQueue::Map(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) noexcept -> void {
        m_khead        = sq_mmap.Offset<unsigned>(p.sq_off.head);
        m_ktail        = sq_mmap.Offset<unsigned>(p.sq_off.tail);
        m_ring_mask    = *sq_mmap.Offset<unsigned>(p.sq_off.ring_mask);
        m_ring_entries = *sq_mmap.Offset<unsigned>(p.sq_off.ring_entries);
        m_kflags       = sq_mmap.Offset<unsigned>(p.sq_off.flags);
        m_entries      = sqe_mmap.Offset<io_uring_sqe>(0);

        if ((p.flags & IORING_SETUP_NO_SQARRAY) == 0) {
            // To keep things simple, we map the slots directly to entries.
            volatile unsigned *array = sq_mmap.Offset<unsigned>(p.sq_off.array);
            for (unsigned i = 0; i < m_ring_entries; ++i) {
                array[i] = i;
            }
        }
    }

    auto SubmissionQueue::Synchronize() const noexcept -> unsigned {
        // Ordering: Release store forms a happens-before relationship with the
        // kernel's acquire load of ktail. This ensures the changes we have made
        // to submission slots will be correctly observed.
        AtomicStore(m_ktail, m_local_tail, std::memory_order_release);

        // Ordering: khead is concurrently written by the kernel. Relaxed ordering
        // is sufficient because the kernel never writes to Submission Queue slots,
        // there is no change to memory that we need to observe other than the head
        // value itself. ktail is safe to load without synchronization because we
        // only ever have a single producer thread per ring.
        return *m_ktail - AtomicLoad(m_khead, std::memory_order_relaxed);
    }

    auto SubmissionQueue::NeedWakeup() const noexcept -> bool {
        // Ordering: The kernel must observe a write to ktail first before it sets
        // this flag. We assume the necessary synchronization is in place prior to
        // calling this function. Therefore, a relaxed load should be sufficient.
        return (AtomicLoad(m_kflags, std::memory_order_relaxed) & IORING_SQ_NEED_WAKEUP) != 0;
    }

    auto SubmissionQueue::NeedCompletionQueueFlush() const noexcept -> bool {
        // Ordering: This is merely an informational function and these flags are
        // not set as a response to an event from our end that the kernel must
        // observe. Therefore, a relaxed load should be sufficient.
        auto flags = AtomicLoad(m_kflags, std::memory_order_relaxed);
        return (flags & (IORING_SQ_CQ_OVERFLOW | IORING_SQ_TASKRUN)) != 0;
    }

    auto SubmissionQueue::HasCapacityFor(unsigned num_submissions) const noexcept -> bool {
        // Ordering: Acquire ordering synchronizes with the kernel's release store
        // of khead and is needed to ensure we don't accidentally override queue
        // slots before the kernel has finished reading them.
        unsigned head = AtomicLoad(m_khead, std::memory_order_acquire);
        return (m_ring_entries - m_local_tail - head) > num_submissions;
    }

    auto SubmissionQueue::PushUnchecked() noexcept -> io_uring_sqe* {
        auto *sqe = &m_entries[m_local_tail & m_ring_mask];
        ++m_local_tail;
        ClearSqe(sqe);
        return sqe;
    }

}
