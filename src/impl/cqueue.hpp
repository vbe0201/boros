#pragma once

#include <linux/io_uring.h>

#include "utils.hpp"

namespace boros::impl {

    struct CompletionEntry {
    private:
        io_uring_cqe *m_cqe;

    public:
        ALWAYS_INLINE explicit CompletionEntry(io_uring_cqe *cqe) noexcept : m_cqe(cqe) {}

        ALWAYS_INLINE auto GetResult() const noexcept -> int {
            return m_cqe->res;
        }

        ALWAYS_INLINE auto GetData() const noexcept -> void* {
            return reinterpret_cast<void*>(m_cqe->user_data);
        }

        ALWAYS_INLINE auto GetFlags() const noexcept -> unsigned {
            return m_cqe->flags;
        }
    };

    class CompletionQueue {
    public:
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
        ALWAYS_INLINE CompletionQueue() noexcept = default;

        auto Map(const io_uring_params &p, const Mmap &cq_mmap) noexcept -> void;

    public:
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

            ALWAYS_INLINE auto operator*() const noexcept -> CompletionEntry {
                auto *cqe = &m_queue->m_entries[m_head & m_mask];
                return CompletionEntry{cqe};
            }

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

        auto begin() noexcept -> Iterator;
        auto end() const noexcept -> Sentinel;
    };

}
