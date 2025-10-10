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
        unsigned *m_khead = nullptr;
        unsigned *m_ktail = nullptr;
        unsigned m_ring_mask = 0;
        unsigned m_ring_entries = 0;
        unsigned *m_kflags = nullptr;
        unsigned *m_koverflow = nullptr; // TODO: Do we even need this?
        CompletionEntry *m_entries = nullptr;

    public:
        ALWAYS_INLINE CompletionQueue() noexcept = default;

        auto Map(const io_uring_params &p, const Mmap &cq_mmap) noexcept -> void;
    };

    class CompletionQueueHandle {
    private:
        unsigned m_head;
        unsigned m_tail;
        CompletionQueue *m_queue;

    public:
        explicit CompletionQueueHandle(CompletionQueue &queue) noexcept;

        ALWAYS_INLINE ~CompletionQueueHandle() noexcept {
            // Ordering: Release store forms a happens-before relationship with the
            // kernel's acquire load of khead. This ensures we are not accessing the
            // Completion Queue slots we consumed anymore by the time the kernel is
            // populating them again.
            AtomicStore(m_queue->m_khead, m_head, std::memory_order_release);
        }

        ALWAYS_INLINE auto Synchronize() noexcept -> void {
            // Ordering: Release store forms a happens-before relationship with the
            // kernel's acquire load of khead. This ensures we are not accessing the
            // Completion Queue slots we consumed anymore by the time the kernel is
            // populating them again.
            AtomicStore(m_queue->m_khead, m_head, std::memory_order_release);

            // Ordering: Acquire load forms a happens-before relationship with the
            // kernel's release store of ktail. This ensures that the kernel has
            // finished populating the reported slots before we start reading them.
            m_tail = AtomicLoad(m_queue->m_ktail, std::memory_order_acquire);
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
