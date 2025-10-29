// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <linux/io_uring.h>

#include "io_uring/mmap.hpp"

namespace boros {

    /// An entry in the submission queue.
    struct SubmissionEntry {
    private:
        io_uring_sqe *m_sqe;

    public:
        explicit SubmissionEntry(io_uring_sqe *sqe) noexcept;
    };

    /// Handle to the io_uring submission queue. This allows the
    /// application to queue I/O operations for the kernel.
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

        SubmissionQueue(const SubmissionQueue&) = delete;
        auto operator=(const SubmissionQueue&) -> SubmissionQueue& = delete;

        /// Maps in the kernel-created submission queue so this
        /// instance can interface with it.
        auto Map(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) noexcept -> void;

        /// Synchronizes the local queue state with the kernel and
        /// returns how many entries must be submitted.
        auto Synchronize() const noexcept -> unsigned;

        /// Whether the SQPOLL thread needs to be woken after it
        /// went into idle state.
        auto NeedWakeup() const noexcept -> bool;

        /// Indicates if there are completions that need to be
        /// flushed to the completion queue through a syscall.
        auto NeedCompletionQueueFlush() const noexcept -> bool;

        /// Indicates if the submission queue has capacity for
        /// n additional entries before pending submissions
        /// must be flushed to the kernel to make space.
        auto HasCapacityFor(unsigned num_submissions) const noexcept -> bool;

        /// Pushes a new entry into the submission queue and
        /// returns its handle which can then be filled with
        /// the details of the I/O operation.
        auto PushUnchecked() noexcept -> SubmissionEntry;
    };

}
