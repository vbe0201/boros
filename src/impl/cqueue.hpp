// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <linux/io_uring.h>

#include "utils.hpp"

namespace boros::impl {

    /// Represents an entry in the completion queue.
    /// Encodes the result of a previously submitted operation.
    struct CompletionEntry {
    private:
        io_uring_cqe *m_cqe;

    public:
        ALWAYS_INLINE explicit CompletionEntry(io_uring_cqe *cqe) noexcept : m_cqe(cqe) {}

        /// Gets the result code of the operation.
        /// Negative values are negated errno codes.
        ALWAYS_INLINE auto GetResult() const noexcept -> int {
            return m_cqe->res;
        }

        /// Gets the user data pointer that was supplied with the
        /// submission entry of the operation.
        ALWAYS_INLINE auto GetData() const noexcept -> void* {
            return reinterpret_cast<void*>(m_cqe->user_data);
        }

        /// Gets the flags of the completion entry.
        ALWAYS_INLINE auto GetFlags() const noexcept -> unsigned {
            return m_cqe->flags;
        }
    };

    /// Handle to the io_uring completion queue.
    /// This is used to obtain results from previously submitted operations.
    class CompletionQueue {
    public:
        /// An input iterator over currently available completion events.
        /// Yields entries while available, and marks them consumed to the
        /// kernel when the iterator instance is destroyed.
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
        CompletionQueue() noexcept = default;

        /// Maps this instance to the kernel-created completion queue
        /// represented by the given mmap handle.
        auto Map(const io_uring_params &p, const Mmap &cq_mmap) noexcept -> void;

    public:
        /// A sentinel that indicates when to stop iteration.
        class Sentinel {
            friend class Iterator;
            unsigned m_tail;

        public:
            ALWAYS_INLINE explicit Sentinel(unsigned tail) noexcept : m_tail(tail) {}
        };

        class Iterator {
        private:
            unsigned m_head;
            unsigned m_mask;
            CompletionQueue *m_queue;

        public:
            using value_type      = CompletionEntry;
            using difference_type = std::ptrdiff_t;

        public:
            ALWAYS_INLINE explicit Iterator(CompletionQueue *queue) noexcept;

            ~Iterator() noexcept;

            auto operator*() const noexcept -> CompletionEntry;

            ALWAYS_INLINE auto operator++() noexcept -> Iterator& {
                ++m_head;
                return *this;
            }

            ALWAYS_INLINE auto operator++(int) noexcept -> void {
                ++*this;
            }

            ALWAYS_INLINE friend auto operator==(const Iterator &a, const Sentinel &b) noexcept -> bool {
                return a.m_head == b.m_tail;
            }

            ALWAYS_INLINE friend auto operator!=(const Iterator &a, const Sentinel &b) noexcept -> bool {
                return a.m_head != b.m_tail;
            }
        };

        /// Gets an iterator over the available completion entries.
        /// Only one iterator instance should be alive at any given time.
        auto begin() noexcept -> Iterator;

        /// Gets a sentinel value that indicates when iteration stops.
        /// Only one sentinel instance should be alive at any given time.
        auto end() const noexcept -> Sentinel;
    };

}
