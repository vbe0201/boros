// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <cstdint>

#include <linux/io_uring.h>

#include "io/mmap.hpp"

namespace boros::io {

    /// An entry in the submission queue. This wraps an io_uring_sqe,
    /// clears its previous state, and allows for configuration of a
    /// new asynchronous I/O operation.
    struct Submission {
        io_uring_sqe *sqe;

        explicit Submission(io_uring_sqe *s) noexcept;

        /// Provides the basic setup for an I/O operation. Certain
        /// types of operations may require additional fields.
        void Prepare(std::uint8_t op, int fd, const void *addr, unsigned len, std::uint64_t off);

        /// Sets an indicator that the file descriptor used with the
        /// operation is a direct descriptor.
        void WithDirectDescriptor() { sqe->flags |= IOSQE_FIXED_FILE; }

        /// Enables the selection of an available buffer from a pool
        /// of provided buffers for the I/O.
        void WithProvidedBuffers(std::uint16_t group) {
            sqe->flags |= IOSQE_BUFFER_SELECT;
            sqe->buf_group = group;
        }
    };

    /// Handle to the io_uring submission queue. The application
    /// fills it with I/O submissions to offload to the kernel.
    class SubmissionQueue {
    private:
        unsigned *m_khead       = nullptr;
        unsigned *m_ktail       = nullptr;
        unsigned m_ring_mask    = 0;
        unsigned m_ring_entries = 0;
        unsigned *m_kflags      = nullptr;
        io_uring_sqe *m_entries = nullptr;

    public:
        SubmissionQueue() noexcept = default;

        SubmissionQueue(const SubmissionQueue &)            = delete;
        SubmissionQueue &operator=(const SubmissionQueue &) = delete;

        /// Maps the kernel-created submission queue into this object.
        void Map(const io_uring_params &p, const Mmap &sq_mmap, const Mmap &sqe_mmap);

        /// Indicates if overflowed completions must be flushed to
        /// the completion queue by entering the kernel.
        bool NeedCompletionQueueFlush() const;

        /// Gets the number of ready submissions in the queue.
        unsigned GetSize() const;

        /// Checks if the queue has capacity for a given number of
        /// additional submissions.
        bool HasCapacityFor(unsigned num) const;

        /// Pushes a new entry into the submission queue and returns
        /// a handle to it. The caller must check queue boundaries
        /// before calling this function.
        Submission Push();
    };

}  // namespace boros::io
