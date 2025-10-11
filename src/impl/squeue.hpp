// This source file is part of the boros project.
// SPDX-License-Identifier: MIT

#pragma once

#include <linux/io_uring.h>

#include "utils.hpp"

namespace boros::impl {

    /// Represents an entry in the submission queue.
    /// Must be populated and then submitted to the kernel.
    struct SubmissionEntry {
    private:
        io_uring_sqe *m_sqe;

    public:
        /// Creates an entry from the underlying io_uring_sqe.
        ALWAYS_INLINE explicit SubmissionEntry(io_uring_sqe *sqe) noexcept;

        /// Sets the given flags on the submission entry.
        ALWAYS_INLINE auto SetFlags(__u8 flags) noexcept -> void {
            m_sqe->flags = flags;
        }

        /// Sets arbitrary user data on the submission entry.
        /// This is then passed to the associated completion entry.
        ALWAYS_INLINE auto SetData(void *user_data) noexcept -> void {
            m_sqe->user_data = reinterpret_cast<std::uintptr_t>(user_data);
        }
    };

    /// Handle to the io_uring submission queue.
    /// This is used to enqueue operations to be performed by the kernel.
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

        /// Maps this instance to the kernel-created submission queue
        /// represented by the given mmap handles.
        auto Map(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) noexcept -> void;

        /// Synchronizes the queue state with the kernel and returns
        /// how many entries must be submitted.
        auto Synchronize() const noexcept -> unsigned;

        /// Whether the SQPOLL thread needs to be woken up by a syscall
        /// after it went to idle. Only applicable when the ring is
        /// created with IORING_SETUP_SQPOLL.
        auto NeedWakeup() const noexcept -> bool;

        /// Indicates whether there are completions that need to be
        /// flushed to the completion queue through a syscall.
        auto NeedCompletionQueueFlush() const noexcept -> bool;

        /// Indicates if the submission queue has capacity for a given
        /// number of additional submissions before pending entries
        /// need to be submitted to the kernel to make more space.
        auto HasCapacityFor(unsigned num_submissions) const noexcept -> bool;

        /// Pushes a new entry into the submission queue and returns
        /// the handle to it. This handle can then be populated with
        /// the details of the I/O operation.
        ///
        /// Note that this function does not do any bounds checks,
        /// users are expected to check HasCapacityFor prior to
        /// calling this.
        auto PushUnchecked() noexcept -> SubmissionEntry;
    };

}
