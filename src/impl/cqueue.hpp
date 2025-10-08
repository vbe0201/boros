#pragma once

#include <linux/io_uring.h>

#include "utils.hpp"

namespace boros::impl {

    class CompletionEntry {
        io_uring_cqe Cqe{};

        ALWAYS_INLINE auto GetResult() const noexcept -> __s32 {
            return Cqe.res;
        }

        ALWAYS_INLINE auto GetData() const noexcept -> void* {
            return reinterpret_cast<void*>(Cqe.user_data);
        }

        ALWAYS_INLINE auto GetFlags() const noexcept -> __u32 {
            return Cqe.flags;
        }
    };

    class CompletionQueueHandle;

    class CompletionQueue {
        friend class CompletionQueueHandle;

    private:
        unsigned *m_khead;
        unsigned *m_ktail;
        unsigned m_ring_mask;
        unsigned m_ring_entries;
        unsigned *m_kflags;
        unsigned *m_koverflow;
        CompletionEntry *m_entries;

    public:
        CompletionQueue(const io_uring_params &p, const Mmap &cq_mmap) noexcept;
    };

    class CompletionQueueHandle {
    private:
        unsigned m_head;
        unsigned m_tail;
        CompletionQueue *m_queue;

    public:
        explicit CompletionQueueHandle(CompletionQueue &queue) noexcept;

        ALWAYS_INLINE ~CompletionQueueHandle() noexcept {
            AtomicStore(m_queue->m_khead, m_head, std::memory_order_release);
        }

        ALWAYS_INLINE auto Synchronize() noexcept -> void {
            AtomicStore(m_queue->m_khead, m_head, std::memory_order_release);
            m_tail = AtomicLoad(m_queue->m_ktail, std::memory_order_acquire);
        }

        ALWAYS_INLINE auto GetOverflow() const noexcept -> unsigned {
            return AtomicLoad(m_queue->m_koverflow, std::memory_order_acquire);
        }

        ALWAYS_INLINE auto GetCapacity() const noexcept -> unsigned {
            return m_queue->m_ring_entries;
        }

        ALWAYS_INLINE auto GetSize() const noexcept -> unsigned {
            return m_tail - m_head;
        }

        ALWAYS_INLINE auto IsEmpty() const noexcept -> bool {
            return this->GetSize() == 0;
        }

        ALWAYS_INLINE auto IsFull() const noexcept -> bool {
            return this->GetSize() == this->GetCapacity();
        }

        ALWAYS_INLINE auto PopUnchecked() -> CompletionEntry& {
            auto &entry = m_queue->m_entries[m_head & m_queue->m_ring_mask];
            ++m_head;
            return entry;
        }
    };

}
