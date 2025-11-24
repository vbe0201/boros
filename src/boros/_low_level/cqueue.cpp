// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "cqueue.hpp"

#include "atomics.hpp"

namespace boros {

    auto CompletionQueue::Map(const io_uring_params &p, const Mmap &cq_mmap) -> void {
        m_khead        = reinterpret_cast<unsigned*>(cq_mmap.ptr + p.cq_off.head);
        m_ktail        = reinterpret_cast<unsigned*>(cq_mmap.ptr + p.cq_off.tail);
        m_ring_mask    = *reinterpret_cast<unsigned*>(cq_mmap.ptr + p.cq_off.ring_mask);
        m_ring_entries = *reinterpret_cast<unsigned*>(cq_mmap.ptr + p.cq_off.ring_entries);
        m_kflags       = reinterpret_cast<unsigned*>(cq_mmap.ptr + p.cq_off.flags);
        m_entries      = reinterpret_cast<io_uring_cqe*>(cq_mmap.ptr + p.cq_off.cqes);
    }

    CompletionQueue::Iterator::~Iterator() {
        AtomicStore(m_queue->m_khead, m_head, std::memory_order_release);
    }

    auto CompletionQueue::Iterator::operator*() const -> Completion {
        auto *cqe = &m_queue->m_entries[m_head & m_queue->m_ring_mask];
        return Completion(cqe);
    }

    auto CompletionQueue::begin() -> Iterator {
        return Iterator(this);
    }

    auto CompletionQueue::end() const -> Sentinel {
        auto tail = AtomicLoad(m_ktail, std::memory_order_acquire);
        return Sentinel{tail};
    }

}
