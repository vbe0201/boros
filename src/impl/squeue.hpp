#pragma once

#include <linux/io_uring.h>

#include "utils.hpp"

namespace boros::impl {

    struct SubmissionEntry {
    private:
        io_uring_sqe *m_sqe;

    public:
        ALWAYS_INLINE explicit SubmissionEntry(io_uring_sqe *sqe) noexcept;

        ALWAYS_INLINE auto SetFlags(__u8 flags) noexcept -> void {
            m_sqe->flags = flags;
        }

        ALWAYS_INLINE auto SetData(void *user_data) noexcept -> void {
            m_sqe->user_data = reinterpret_cast<std::uintptr_t>(user_data);
        }
    };

    class SubmissionQueue {
    private:
        unsigned *m_khead = nullptr;
        unsigned *m_ktail = nullptr;
        unsigned m_ring_mask = 0;
        unsigned m_ring_entries = 0;
        unsigned *m_kflags = nullptr;
        io_uring_sqe *m_entries = nullptr;
        unsigned m_local_tail = 0;

    public:
        SubmissionQueue() noexcept = default;

        auto Map(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) noexcept -> void;

        auto Synchronize() const noexcept -> unsigned;

        auto NeedWakeup() const noexcept -> bool;

        auto NeedCompletionQueueFlush() const noexcept -> bool;

        auto HasCapacityFor(unsigned num_submissions) const noexcept -> bool;

        auto PushUnchecked() noexcept -> SubmissionEntry;
    };

}
