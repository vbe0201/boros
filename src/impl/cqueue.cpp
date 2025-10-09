#include "cqueue.hpp"

namespace boros::impl {

    auto CompletionQueue::Map(const io_uring_params &p, const Mmap &cq_mmap) noexcept -> void {
        m_khead        = cq_mmap.Offset<unsigned>(p.cq_off.head);
        m_ktail        = cq_mmap.Offset<unsigned>(p.cq_off.tail);
        m_ring_mask    = *cq_mmap.Offset<unsigned>(p.cq_off.ring_mask);
        m_ring_entries = *cq_mmap.Offset<unsigned>(p.cq_off.ring_entries);
        m_kflags       = cq_mmap.Offset<unsigned>(p.cq_off.flags);
        m_koverflow    = cq_mmap.Offset<unsigned>(p.cq_off.overflow);
        m_entries      = cq_mmap.Offset<CompletionEntry>(p.cq_off.cqes);
    }

    CompletionQueueHandle::CompletionQueueHandle(CompletionQueue& queue) noexcept
        : m_head(*queue.m_khead),
          m_tail(AtomicLoad(queue.m_ktail, std::memory_order_acquire)),
          m_queue(&queue) {}

}
