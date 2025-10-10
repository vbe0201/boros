#pragma once

#include <linux/io_uring.h>

#include "utils.hpp"

namespace boros::impl {

    struct SubmissionEntry {
        io_uring_sqe Sqe{};

        ALWAYS_INLINE auto SetFlags(__u8 flags) noexcept -> void {
            Sqe.flags = flags;
        }

        ALWAYS_INLINE auto GetData() const noexcept -> void* {
            return reinterpret_cast<void*>(Sqe.user_data);
        }

        ALWAYS_INLINE auto SetData(void *user_data) noexcept -> void {
            Sqe.user_data = reinterpret_cast<std::uintptr_t>(user_data);
        }
    };

    class SubmissionQueueHandle;

    class SubmissionQueue {
        friend class SubmissionQueueHandle;

    private:
        unsigned *m_khead = nullptr;
        unsigned *m_ktail = nullptr;
        unsigned m_ring_mask = 0;
        unsigned m_ring_entries = 0;
        unsigned *m_kflags = nullptr;
        unsigned *m_kdropped = nullptr; // TODO: Do we even care about this?
        SubmissionEntry *m_entries = nullptr;

    public:
        ALWAYS_INLINE SubmissionQueue() noexcept = default;

        auto Map(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) noexcept -> void;

        ALWAYS_INLINE auto GetUnsubmittedEntries() const noexcept -> unsigned {
            // Ordering: khead is concurrently written by the kernel. Relaxed ordering
            // is sufficient because the kernel never writes to Submission Queue slots,
            // it merely bumps khead when it is done consuming a submission. ktail is
            // safe to load without synchronization because we only ever have a single
            // producer thread per ring.
            return *m_ktail - AtomicLoad(m_khead, std::memory_order_relaxed);
        }

        ALWAYS_INLINE auto NeedWakeup() const noexcept -> bool {
            // Ordering: The kernel must observe a write to ktail first before it sets
            // this flag. This is enforced through a sequential consistency fence and
            // that already provides sufficient synchronization for the relaxed load
            // to observe changes to kflags.
            return (AtomicLoad(m_kflags, std::memory_order_relaxed) & IORING_SQ_NEED_WAKEUP) != 0;
        }

        ALWAYS_INLINE auto NeedCompletionQueueFlush() const noexcept -> bool {
            // Ordering: This is merely an informational function and these flags are
            // not set as a response to a Submission Queue write in SQPOLL mode. There
            // is no need for a stricter ordering than relaxed.
            auto flags = AtomicLoad(m_kflags, std::memory_order_relaxed);
            return (flags & (IORING_SQ_CQ_OVERFLOW | IORING_SQ_TASKRUN)) != 0;
        }
    };

    class SubmissionQueueHandle {
    private:
        unsigned m_head;
        unsigned m_tail;
        SubmissionQueue *m_queue;

    public:
        explicit SubmissionQueueHandle(SubmissionQueue &queue) noexcept;

        ALWAYS_INLINE ~SubmissionQueueHandle() noexcept {
            // Ordering: Release store forms a happens-before relationship with the
            // kernel's acquire load of ktail. This ensures the kernel correctly
            // sees the submissions we've written prior to bumping ktail.
            AtomicStore(m_queue->m_ktail, m_tail, std::memory_order_release);
        }

        ALWAYS_INLINE auto Synchronize() noexcept -> void {
            // Ordering: Release store forms a happens-before relationship with the
            // kernel's acquire load of ktail. This ensures the kernel correctly
            // sees the submissions we've written prior to bumping ktail.
            AtomicStore(m_queue->m_ktail, m_tail, std::memory_order_release);

            // Ordering: Acquire load forms a happens-before relationship with the
            // kernel's release store of khead. This ensures the kernel has finished
            // reading submissions from the released slots before we fill them again.
            m_head = AtomicLoad(m_queue->m_khead, std::memory_order_acquire);
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

        ALWAYS_INLINE auto PushUnchecked(SubmissionEntry &entry) noexcept -> void {
            m_queue->m_entries[m_tail & m_queue->m_ring_mask] = entry;
            ++m_tail;
        }
    };

}
