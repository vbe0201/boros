// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "io_uring/cqueue.hpp"

#include "io_uring/atomics.hpp"

namespace boros {

    auto CompletionQueue::Map(const io_uring_params &p, const Mmap &cq_mmap) noexcept -> void {
        m_khead        = cq_mmap.GetPointer<unsigned>(p.cq_off.head);
        m_ktail        = cq_mmap.GetPointer<unsigned>(p.cq_off.tail);
        m_ring_mask    = *cq_mmap.GetPointer<unsigned>(p.cq_off.ring_mask);
        m_ring_entries = *cq_mmap.GetPointer<unsigned>(p.cq_off.ring_entries);
        m_kflags       = cq_mmap.GetPointer<unsigned>(p.cq_off.flags);
        m_entries      = cq_mmap.GetPointer<io_uring_cqe>(p.cq_off.cqes);
    }

    CompletionQueue::Iterator::Iterator(CompletionQueue *queue) noexcept
        : m_queue(queue), m_head(*m_queue->m_khead) {}

    CompletionQueue::Iterator::~Iterator() noexcept {
        // Ordering: Release store forms a happens-before relationship
        // with the kernel's acquire load of khead. This ensures reads
        // from the application are done before the kernel fills these
        // completion slots again.
        AtomicStore(m_queue->m_khead, m_head, std::memory_order_release);
    }

    auto CompletionQueue::Iterator::operator*() const noexcept -> CompletionEntry {
        auto *cqe = &m_queue->m_entries[m_head & m_queue->m_ring_mask];
        return CompletionEntry(cqe);
    }

    auto CompletionQueue::begin() noexcept -> Iterator {
        return Iterator(this);
    }

    auto CompletionQueue::end() const noexcept -> Sentinel {
        // Ordering: Acquire load forms a happens-before relationship
        // with the kernel's release store of ktail. This ensures the
        // kernel is not writing to the queue slots anymore when we
        // start accessing them.
        auto tail = AtomicLoad(m_ktail, std::memory_order_acquire);
        return Sentinel{tail};
    }

}
