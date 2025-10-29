// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <linux/io_uring.h>

#include "io_uring/mmap.hpp"

namespace boros {

    /// An entry in the completion queue.
    class CompletionEntry {
    private:
        io_uring_cqe *m_cqe;

    public:
        ALWAYS_INLINE explicit CompletionEntry(io_uring_cqe *cqe) noexcept : m_cqe(cqe) {}
    };

    /// Handle to the io_uring completion queue. Used to obtain results
    /// for previously submitted operations from the kernel.
    class CompletionQueue {
    public:
        /// Input iterator over available completions. Yields entries
        /// while available, and marks them consumed to the kernel
        /// when the iterator instance is destroyed.
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

        CompletionQueue(const CompletionQueue&) = delete;
        auto operator=(const CompletionQueue&) = delete;

        /// Maps in the kernel-created completion queue so this
        /// instance can interface with it.
        auto Map(const io_uring_params &p, const Mmap &cq_mmap) noexcept -> void;

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
            using value_type      = CompletionEntry;
            using difference_type = std::ptrdiff_t;

        public:
            explicit Iterator(CompletionQueue *queue) noexcept;

            ~Iterator() noexcept;

            auto operator*() const noexcept -> CompletionEntry;

            ALWAYS_INLINE auto operator++() noexcept -> Iterator& {
                ++m_head;
                return *this;
            }

            ALWAYS_INLINE auto operator++(int) noexcept -> void {
                ++*this;
            }

            ALWAYS_INLINE friend auto operator==(const Iterator &a, const Sentinel &b) noexcept {
                return a.m_head == b.tail;
            }

            ALWAYS_INLINE friend auto operator!=(const Iterator &a, const Sentinel &b) noexcept {
                return a.m_head != b.tail;
            }
        };

        /// Gets an iterator over available completions. Only one
        /// iterator instance should be alive at any given time.
        auto begin() noexcept -> Iterator;

        /// Gets a sentinel that indicates when iteration stops.
        auto end() const noexcept -> Sentinel;
    };

}
