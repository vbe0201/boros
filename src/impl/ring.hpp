#pragma once

#include "cqueue.hpp"
#include "squeue.hpp"

namespace boros::impl {

    class Ring {
    private:
        SubmissionQueue m_submission_queue;
        CompletionQueue m_completion_queue;
        int m_ring_fd = -1;
        int m_enter_fd = -1;
        unsigned m_flags = 0;
        unsigned m_features = 0;
        Mmap m_sq_mmap;
        Mmap m_sqe_mmap;
        Mmap m_cq_mmap;

    public:
        Ring() noexcept = default;
        ~Ring() noexcept;

        auto Create(unsigned entries, io_uring_params &p) noexcept -> int;
        auto CreateWithFile(int fd, io_uring_params &p) noexcept -> int;
    };

}
