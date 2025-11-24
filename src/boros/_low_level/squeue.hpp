// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <cstdint>

#include <linux/io_uring.h>

#include "macros.hpp"
#include "mmap.hpp"

namespace boros {

    /// An entry in the submission queue. This wraps an io_uring_sqe,
    /// clears its previous state, and allows for  configuration of
    /// a new asynchronous I/O operation.
    struct Submission {
        io_uring_sqe *sqe;

        ALWAYS_INLINE explicit Submission(io_uring_sqe *s) : sqe(s) {
            sqe->flags       = 0;
            sqe->ioprio      = 0;
            sqe->rw_flags    = 0;
            sqe->buf_index   = 0;
            sqe->personality = 0;
            sqe->file_index  = 0;
            sqe->addr3       = 0;
            sqe->__pad2[0]   = 0;
        }

        ALWAYS_INLINE auto Prepare(std::uint8_t op, int fd, const void *addr, unsigned len, std::uint64_t off) -> void {
            sqe->opcode = op;
            sqe->fd     = fd;
            sqe->off    = off;
            sqe->addr   = reinterpret_cast<std::uintptr_t>(addr);
            sqe->len    = len;
        }

        ALWAYS_INLINE auto WithDirectDescriptor() -> void {
            sqe->flags |= IOSQE_FIXED_FILE;
        }

        ALWAYS_INLINE auto WithProvidedBuffers(std::uint16_t group) -> void {
            sqe->flags |= IOSQE_BUFFER_SELECT;
            sqe->buf_group = group;
        }
    };

    /// Handle to the io_uring submission queue. The application fills
    /// its entries to queue I/O operations for the kernel.
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
        SubmissionQueue() = default;

        SubmissionQueue(const SubmissionQueue&) = delete;
        auto operator=(const SubmissionQueue&) -> SubmissionQueue& = delete;

        /// Maps the kernel-created submission queue into this object.
        auto Map(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap) -> void;

        /// Synchronizes the local queue state with the kernel and
        /// returns how many entries can be submitted.
        auto Synchronize() const -> unsigned;

        /// Indicates if there are completions that need to be
        /// flushed to the completion queue through a syscall.
        auto NeedCompletionQueueFlush() const -> bool;

        /// Checks if the queue has capacity for n more entries.
        auto HasCapacityFor(unsigned num) const -> bool;

        /// Pushes a new entry into the submission queue and returns it.
        /// The caller must check queue boundaries before calling.
        auto PushUnchecked() -> Submission;
    };

}
