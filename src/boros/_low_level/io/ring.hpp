// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <chrono>
#include <optional>

#include "io/cqueue.hpp"
#include "io/squeue.hpp"

namespace boros::io {

    /// A driver for an io_uring instance. Optimized for a single
    /// producer thread in an event loop architecture.
    class Ring {
    private:
        SubmissionQueue m_submission_queue;
        CompletionQueue m_completion_queue;
        unsigned m_flags    = 0;
        unsigned m_features = 0;
        int m_ring_fd       = -1;
        bool m_registered   = false;
        int m_enter_fd      = -1;
        Mmap m_scq_map;
        Mmap m_sqe_map;

    public:
        Ring() noexcept = default;
        ~Ring();

    private:
        int Setup(unsigned entries, io_uring_params &p);
        int SetupWithFile(int fd, io_uring_params &p);

    public:
        /// Creates a new io_uring instance with the given config
        /// parameters and maps it to this object.
        int Initialize(unsigned sq_entries, unsigned cq_entries);

        /// Tears down this io_uring instance. The object may be
        /// reinitialized by another call to Initialize.
        void Finalize();

        /// Checks if this instance is currently initialized.
        bool IsInitialized() const { return m_scq_map.IsMapped(); }

        /// Gets the configuration flags of the instance.
        unsigned GetFlags() const { return m_flags; }

        /// Gets the supported io_uring feature flags.
        unsigned GetFeatures() const { return m_features; }

        /// Gets the file descriptor associated with the io_uring,
        /// or -1 if it was closed.
        int GetRingFd() const { return m_ring_fd; }

        /// Gets a handle to the submission queue of this ring.
        SubmissionQueue &GetSubmissionQueue() { return m_submission_queue; }

        /// Gets a handle to the completion queue of this ring.
        CompletionQueue &GetCompletionQueue() { return m_completion_queue; }

    private:
        int Register(unsigned opcode, const void *arg, unsigned nargs) const;

    public:
        /// Enables a ring instance from disabled creation state.
        int Enable() const;

        /// Installs the file descriptor of the ring as a direct
        /// descriptor. This reduces overhead in system calls.
        int RegisterRingFd();

        /// Uninstalls the file descriptor of the ring from the
        /// direct descriptors set.
        int UnregisterRingFd();

    private:
        int Enter(unsigned want, __kernel_timespec *ts) const;

    public:
        /// Submits pending submissions to the kernel without waiting
        /// for completions. Returns the number of submissions.
        int Submit() const;

        /// Submits pending submissions to the kernel and waits for
        /// want completions. An optional timeout can be provided
        /// for the wait.
        int Wait(unsigned want, std::optional<std::chrono::milliseconds> timeout = std::nullopt) const;
    };

}  // namespace boros::io
