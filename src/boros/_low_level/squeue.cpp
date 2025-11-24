// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "squeue.hpp"

#include "atomics.hpp"

namespace boros {

    auto SubmissionQueue::Map(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) -> void {
        m_khead        = reinterpret_cast<unsigned*>(sq_mmap.ptr + p.sq_off.head);
        m_ktail        = reinterpret_cast<unsigned*>(sq_mmap.ptr + p.sq_off.tail);
        m_ring_mask    = *reinterpret_cast<unsigned*>(sq_mmap.ptr + p.sq_off.ring_mask);
        m_ring_entries = *reinterpret_cast<unsigned*>(sq_mmap.ptr + p.sq_off.ring_entries);
        m_kflags       = reinterpret_cast<unsigned*>(sq_mmap.ptr + p.sq_off.flags);
        m_entries      = reinterpret_cast<io_uring_sqe*>(sqe_mmap.ptr);
        m_local_tail   = *m_ktail;

        if ((p.flags & IORING_SETUP_NO_SQARRAY) == 0) {
            // To keep things simple, map the slots directly to entries.
            volatile unsigned *array = reinterpret_cast<unsigned*>(sq_mmap.ptr + p.sq_off.array);
            for (unsigned i = 0; i < m_ring_entries; ++i) {
                array[i] = i;
            }
        }
    }

    auto SubmissionQueue::Synchronize() const -> unsigned {
        *m_ktail = m_local_tail;
        return m_local_tail - *m_khead;
    }

    auto SubmissionQueue::NeedCompletionQueueFlush() const -> bool {
        auto flags = AtomicLoad(m_kflags, std::memory_order_relaxed);
        return (flags & (IORING_SQ_CQ_OVERFLOW | IORING_SQ_TASKRUN)) != 0;
    }

    auto SubmissionQueue::HasCapacityFor(unsigned num) const -> bool {
        return (m_ring_entries - m_local_tail - *m_khead) > num;
    }

    auto SubmissionQueue::PushUnchecked() -> Submission {
        auto *sqe = &m_entries[m_local_tail & m_ring_mask];
        ++m_local_tail;
        return Submission(sqe);
    }

}
