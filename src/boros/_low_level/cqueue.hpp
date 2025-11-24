// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <cstdint>

#include <linux/io_uring.h>

#include "macros.hpp"
#include "mmap.hpp"

namespace boros {

    /// An entry in the completion queue. This wraps an io_uring_cqe
    /// and exposes the kernel result for a previous submission.
    struct Completion {
        io_uring_cqe *cqe;

        ALWAYS_INLINE explicit Completion(io_uring_cqe *c) : cqe(c) {}

        ALWAYS_INLINE auto IsFinished() const -> bool {
            return (cqe->flags & IORING_CQE_F_MORE) == 0;
        }

        ALWAYS_INLINE auto UsesProvidedBuffer() const -> bool {
            return (cqe->flags & IORING_CQE_F_BUFFER) != 0;
        }

        ALWAYS_INLINE auto HasMoreIncomingData() const -> bool {
            return (cqe->flags & IORING_CQE_F_BUF_MORE) != 0;
        }

        ALWAYS_INLINE auto GetBufferId() const -> std::uint16_t {
            return cqe->flags >> IORING_CQE_BUFFER_SHIFT;
        }
    };

    /// Handle to the io_uring completion queue. The application obtains
    /// the kernel results from previously submitted operations from it.
    class CompletionQueue {
    public:
        /// Iterator over available completions. Yields entries while
        /// available, and marks them consumed to the kernel when the
        /// instance is destroyed.
        class Iterator;
        friend class Iterator;

    private:
        unsigned *m_khead = nullptr;
        unsigned *m_ktail = nullptr;
        unsigned m_ring_mask = 0;
        unsigned m_ring_entries = 0;
        unsigned *m_kflags = nullptr;
        io_uring_cqe *m_entries = nullptr;

    public:
        CompletionQueue() = default;

        CompletionQueue(const CompletionQueue&) = delete;
        auto operator=(const CompletionQueue&) = delete;

        /// Maps the kernel-created completion queue into this object.
        auto Map(const io_uring_params &p, const Mmap &cq_mmap) -> void;

    public:
        /// Sentinel value that indicates when to stop iteration.
        struct Sentinel {
            unsigned tail;
        };

        class Iterator {
        private:
            CompletionQueue *m_queue;
            unsigned m_head;

        public:
            using value_type      = Completion;
            using difference_type = std::ptrdiff_t;

        public:
            ALWAYS_INLINE explicit Iterator(CompletionQueue *q)
                : m_queue(q), m_head(*q->m_khead) {}

            ~Iterator();

            auto operator*() const -> Completion;

            ALWAYS_INLINE auto operator++() -> Iterator& {
                ++m_head;
                return *this;
            }

            ALWAYS_INLINE auto operator++(int) -> void {
                ++*this;
            }

            ALWAYS_INLINE friend auto operator==(const Iterator &a, const Sentinel &b) -> bool {
                return a.m_head == b.tail;
            }

            ALWAYS_INLINE friend auto operator!=(const Iterator &a, const Sentinel &b) -> bool {
                return a.m_head != b.tail;
            }
        };

        /// Gets an iterator over available completions. Only one
        /// instance should be alive at any given time.
        auto begin() -> Iterator;

        /// Gets a sentinel value to signal the end of iteration.
        auto end() const -> Sentinel;
    };

}
