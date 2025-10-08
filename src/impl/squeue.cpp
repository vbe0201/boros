#include "squeue.hpp"

namespace boros::impl {

    SubmissionQueue::SubmissionQueue(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) noexcept
        : m_khead(sq_mmap.Offset<unsigned>(p.sq_off.head)),
          m_ktail(sq_mmap.Offset<unsigned>(p.sq_off.tail)),
          m_ring_mask(*sq_mmap.Offset<unsigned>(p.sq_off.ring_mask)),
          m_ring_entries(*sq_mmap.Offset<unsigned>(p.sq_off.ring_entries)),
          m_kflags(sq_mmap.Offset<unsigned>(p.sq_off.flags)),
          m_kdropped(sq_mmap.Offset<unsigned>(p.sq_off.dropped)),
          m_entries(static_cast<SubmissionEntry*>(sqe_mmap.Address))
    {
        /* To keep things simple, we map the slots directly to entries. */
        volatile unsigned *array = sq_mmap.Offset<unsigned>(p.sq_off.array);
        for (unsigned i = 0; i < m_ring_entries; ++i) {
            array[i] = i;
        }
    }

    SubmissionQueueHandle::SubmissionQueueHandle(SubmissionQueue& queue) noexcept
        : m_head(AtomicLoad(queue.m_khead, std::memory_order_acquire)),
          m_tail(*queue.m_ktail),
          m_queue(&queue) {}

}
