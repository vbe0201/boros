#pragma once

#include <cstdint>

#include <linux/io_uring.h>

#include "utils.hpp"

namespace boros::impl {

    struct Entry {
        io_uring_sqe Sqe;

        auto SetFlags(__u8 flags) noexcept -> void {
            Sqe.flags = flags;
        }

        auto GetData() const noexcept -> void* {
            return reinterpret_cast<void*>(Sqe.user_data);
        }

        auto SetData(void *user_data) noexcept -> void {
            Sqe.user_data = reinterpret_cast<std::uintptr_t>(user_data);
        }

        auto SetPersonality(__u16 personality) -> void {
            Sqe.personality = personality;
        }
    };

    class SubmissionQueueHandle;

    class SubmissionQueue {
        friend class SubmissionQueueHandle;

    private:
        unsigned *m_khead;
        unsigned *m_ktail;
        unsigned m_ring_mask;
        unsigned m_ring_entries;
        unsigned *m_kflags;
        unsigned *m_kdropped;
        Entry *m_entries;

    public:
        SubmissionQueue(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) noexcept;
    };

    class SubmissionQueueHandle {
    private:
        unsigned m_head;
        unsigned m_tail;
        SubmissionQueue *m_queue;

    public:
        explicit SubmissionQueueHandle(SubmissionQueue &queue) noexcept;

        ~SubmissionQueueHandle() noexcept;

        ALWAYS_INLINE auto Synchronize() noexcept -> void {
            AtomicStore(m_queue->m_ktail, m_tail, std::memory_order_release);
            m_head = AtomicLoad(m_queue->m_khead, std::memory_order_acquire);
        }

        ALWAYS_INLINE auto NeedWakeup() const noexcept -> bool {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            return (AtomicLoad(m_queue->m_kflags, std::memory_order_relaxed) & IORING_SQ_NEED_WAKEUP) != 0;
        }

        ALWAYS_INLINE auto GetDropped() const noexcept -> unsigned {
            return AtomicLoad(m_queue->m_kdropped, std::memory_order_acquire);
        }

        ALWAYS_INLINE auto HasCompletionQueueOverflow() const noexcept -> bool {
            return (AtomicLoad(m_queue->m_kflags, std::memory_order_acquire) & IORING_SQ_CQ_OVERFLOW) != 0;
        }

        ALWAYS_INLINE auto HasPendingTaskrun() const noexcept -> bool {
            return (AtomicLoad(m_queue->m_kflags, std::memory_order_acquire) & IORING_SQ_TASKRUN) != 0;
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

        ALWAYS_INLINE auto PushUnchecked(Entry &entry) noexcept -> void {
            m_queue->m_entries[m_tail & m_queue->m_ring_mask] = entry;
            ++m_tail;
        }
    };

}
