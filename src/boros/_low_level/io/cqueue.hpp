// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <cstdint>

#include <linux/io_uring.h>

#include "mmap.hpp"

namespace boros::io {

    /// An entry in the completion queue. This reports the result of
    /// an I/O operation that was executed by the kernel.
    struct Completion {
        io_uring_cqe *cqe;

        explicit Completion(io_uring_cqe *c) noexcept : cqe(c) {}

        /// Gets the result code of the operation. May be a negative
        /// errno value on error.
        int GetResult() const { return cqe->res; }

        /// Whether an operation will produce more completions after
        /// this one. Relevant with multishot operations.
        bool IsFinished() const { return (cqe->flags & IORING_CQE_F_MORE) != 0; }

        /// Whether an operation was using a provided buffer.
        bool UsesProvidedBuffer() const { return (cqe->flags & IORING_CQE_F_BUFFER) != 0; }

        /// Whether more completions using memory from the same
        /// provided buffer should be expected.
        bool HasMoreIncomingData() const { return (cqe->flags & IORING_CQE_F_BUF_MORE) != 0; }

        /// Gets the ID of a buffer from a provided buffer pool, if
        /// one was used by the operation.
        std::uint16_t GetBufferId() const { return cqe->flags >> IORING_CQE_BUFFER_SHIFT; }
    };

    /// Handle to the io_uring completion queue. The application
    // obtains the results from previously submitted operations
    /// from it.
    class CompletionQueue {
    public:
        class Iterator;
        friend class Iterator;

        struct Sentinel {
            unsigned tail;
        };

    private:
        unsigned *m_khead       = nullptr;
        unsigned *m_ktail       = nullptr;
        unsigned m_ring_mask    = 0;
        unsigned m_ring_entries = 0;
        unsigned *m_kflags      = nullptr;
        io_uring_cqe *m_entries = nullptr;

    public:
        CompletionQueue() noexcept = default;

        CompletionQueue(const CompletionQueue &)            = delete;
        CompletionQueue &operator=(const CompletionQueue &) = delete;

        /// Maps the kernel-created completion queue into this object.
        void Map(const io_uring_params &p, const Mmap &cq_mmap);

    public:
        class Iterator {
        private:
            CompletionQueue *m_queue;
            unsigned m_head;

        public:
            using value_type      = Completion;
            using difference_type = std::ptrdiff_t;

        public:
            explicit Iterator(CompletionQueue *q) : m_queue(q), m_head(*q->m_khead) {}

            ~Iterator();

            Completion operator*() const;

            Iterator &operator++() {
                ++m_head;
                return *this;
            }

            void operator++(int) { ++*this; }

            friend bool operator==(const Iterator &a, const Sentinel &b) { return a.m_head == b.tail; }
            friend bool operator!=(const Iterator &a, const Sentinel &b) { return a.m_head != b.tail; }
        };

        /// Gets an iterator over available completions. Only one
        /// instance should be alive at any given time.
        Iterator begin();

        /// Gets a sentinel value to signal the end of iteration.
        Sentinel end() const;
    };

}  // namespace boros::io
