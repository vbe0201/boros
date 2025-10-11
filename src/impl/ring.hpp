#pragma once

#include "cqueue.hpp"
#include "squeue.hpp"

namespace boros::impl {

    class Ring {
    private:
        SubmissionQueue m_submission_queue;
        CompletionQueue m_completion_queue;
        unsigned m_flags = 0;
        unsigned m_features = 0;
        int m_ring_fd = -1;
        bool m_registered = false;
        int m_enter_fd = -1;
        Mmap m_sq_mmap;
        Mmap m_sqe_mmap;
        Mmap m_cq_mmap;

    public:
        Ring() noexcept = default;
        ~Ring() noexcept;

        auto Create(unsigned entries, io_uring_params &p) noexcept -> int;
        auto CreateWithFile(int fd, io_uring_params &p) noexcept -> int;

        ALWAYS_INLINE auto GetFlags() const noexcept -> unsigned {
            return m_flags;
        }

        ALWAYS_INLINE auto GetFeatures() const noexcept -> unsigned {
            return m_features;
        }

        ALWAYS_INLINE auto GetSubmissionQueue() noexcept -> SubmissionQueue& {
            return m_submission_queue;
        }

        ALWAYS_INLINE auto GetCompletionQueue() noexcept -> CompletionQueue& {
            return m_completion_queue;
        }

    private:
        auto RegisterRaw(unsigned int opcode, const void *arg, unsigned int num_args) const noexcept -> int;

    public:
        auto RegisterFilesSparse(unsigned num_files) const noexcept -> int;
        auto UnregisterFiles() const noexcept -> int;

        auto RegisterEventFd(int fd) const noexcept -> int;
        auto RegisterEventFdAsync(int fd) const noexcept -> int;
        auto UnregisterEventFd() const noexcept -> int;

        auto RegisterProbe(io_uring_probe *probe) const noexcept -> int;

        auto Enable() const noexcept -> int;

        auto RegisterRingFd() noexcept -> int;
        auto UnregisterRingFd() noexcept -> int;
        auto CloseRingFd() noexcept -> int;

        auto RegisterNapi(io_uring_napi *napi) const noexcept -> int;
        auto UnregisterNapi(io_uring_napi *napi) const noexcept -> int;

        auto RegisterBufRing(io_uring_buf_reg *reg) const noexcept -> int;
        auto UnregisterBufRing(int bgid) const noexcept -> int;
        auto GetBufRingHead(int buf_group, std::uint16_t *head) const noexcept -> int;

    public:
        auto SubmitAndWait(unsigned want) const noexcept -> int;
        auto Submit() const noexcept -> int;
    };

}
