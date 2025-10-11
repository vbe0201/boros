// This source file is part of the boros project.
// SPDX-License-Identifier: MIT

#include "cqueue.hpp"

namespace boros::impl {

    auto CompletionQueue::Map(const io_uring_params &p, const Mmap &cq_mmap) noexcept -> void {
        m_khead        = cq_mmap.Offset<unsigned>(p.cq_off.head);
        m_ktail        = cq_mmap.Offset<unsigned>(p.cq_off.tail);
        m_ring_mask    = *cq_mmap.Offset<unsigned>(p.cq_off.ring_mask);
        m_ring_entries = *cq_mmap.Offset<unsigned>(p.cq_off.ring_entries);
        m_kflags       = cq_mmap.Offset<unsigned>(p.cq_off.flags);
        m_entries      = cq_mmap.Offset<io_uring_cqe>(p.cq_off.cqes);
    }

    CompletionQueue::Iterator::Iterator(CompletionQueue *queue) noexcept
        : m_head(*queue->m_khead), m_mask(queue->m_ring_mask), m_queue(queue) {}

    CompletionQueue::Iterator::~Iterator() noexcept {
        // Ordering: Release store forms a happens-before relationship with the
        // kernel's acquire load of khead. This ensures the kernel only writes
        // to the completion slots again when we are not accessing them anymore.
        AtomicStore(m_queue->m_khead, m_head, std::memory_order_release);
    }

    auto CompletionQueue::Iterator::operator*() const noexcept -> CompletionEntry {
        auto *cqe = &m_queue->m_entries[m_head & m_mask];
        return CompletionEntry{cqe};
    }

    auto CompletionQueue::begin() noexcept -> Iterator {
        return Iterator{this};
    }

    auto CompletionQueue::end() const noexcept -> Sentinel {
        // Ordering: Acquire load forms a happens-before relationship with the
        // kernel's release store of ktail. This ensures that the kernel is not
        // accessing the queue slots anymore by the time we start reading them.
        const unsigned tail = AtomicLoad(m_ktail, std::memory_order_acquire);
        return Sentinel{tail};
    }

}
