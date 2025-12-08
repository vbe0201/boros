// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#include "io/cqueue.hpp"

#include "io/atomics.hpp"

namespace boros::io {

    void CompletionQueue::Map(const io_uring_params &p, const Mmap &cq_mmap) {
        m_khead        = reinterpret_cast<unsigned *>(cq_mmap.ptr + p.cq_off.head);
        m_ktail        = reinterpret_cast<unsigned *>(cq_mmap.ptr + p.cq_off.tail);
        m_ring_mask    = *reinterpret_cast<unsigned *>(cq_mmap.ptr + p.cq_off.ring_mask);
        m_ring_entries = *reinterpret_cast<unsigned *>(cq_mmap.ptr + p.cq_off.ring_entries);
        m_kflags       = reinterpret_cast<unsigned *>(cq_mmap.ptr + p.cq_off.flags);
        m_entries      = reinterpret_cast<io_uring_cqe *>(cq_mmap.ptr + p.cq_off.cqes);
    }

    CompletionQueue::Iterator::~Iterator() {
        // Ordering: Storing khead with release ordering ensures we
        // have finished reading the slots before the kernel writes
        // to them again.
        impl::AtomicStore(m_queue->m_khead, m_head, std::memory_order_release);
    }

    Completion CompletionQueue::Iterator::operator*() const {
        auto *cqe = &m_queue->m_entries[m_head & m_queue->m_ring_mask];
        return Completion{cqe};
    }

    CompletionQueue::Iterator CompletionQueue::begin() {
        return Iterator{this};
    }

    CompletionQueue::Sentinel CompletionQueue::end() const {
        // Ordering: Loading ktail with acquire ordering ensures we
        // are only reading the slots after the kernel has finished
        // writing to them.
        auto tail = impl::AtomicLoad(m_ktail, std::memory_order_acquire);
        return Sentinel{tail};
    }

}  // namespace boros::io
