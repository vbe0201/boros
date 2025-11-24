// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include <chrono>
#include <optional>

#include "cqueue.hpp"
#include "macros.hpp"
#include "squeue.hpp"

namespace boros {

    /// A driver for an io_uring instance. Optimized for a single
    /// producer thread in an event loop setting.
    class IoRing {
    private:
        SubmissionQueue m_submission_queue;
        CompletionQueue m_completion_queue;
        unsigned m_flags = 0;
        unsigned m_features = 0;
        int m_ring_fd = -1;
        bool m_registered = false;
        int m_enter_fd = -1;
        Mmap m_scq_map;
        Mmap m_sqe_map;

    public:
        IoRing() = default;
        ~IoRing();

    private:
        auto Setup(unsigned entries, io_uring_params &p) -> int;
        auto SetupWithFile(int fd, io_uring_params &p) -> int;

    public:
        /// Creates a new io_uring instance with the given configuration
        /// parameters and maps it to this object.
        auto Initialize(unsigned sq_entries, unsigned cq_entries) -> int;

        /// Tears down this io_uring instance. No functions may be
        /// called on this object anymore until it is reinitialized
        /// with another io_uring.
        auto Finalize() -> void;

        /// Checks if this instance is currently initialized.
        ALWAYS_INLINE auto IsInitialized() const -> bool {
            return m_scq_map.IsMapped();
        }

        /// Gets the configuration flags of the ring.
        ALWAYS_INLINE auto GetFlags() const -> unsigned {
            return m_flags;
        }

        /// Gets the supported io_uring feature flags.
        ALWAYS_INLINE auto GetFeatures() const -> unsigned {
            return m_features;
        }

        /// Gets the file descriptor associated with the io_uring,
        /// or -1 if it was closed already.
        ALWAYS_INLINE auto GetRingFd() const -> int {
            return m_ring_fd;
        }

        /// Gets a handle to the submission queue of this ring.
        ALWAYS_INLINE auto GetSubmissionQueue() -> SubmissionQueue& {
            return m_submission_queue;
        }

        /// Gets a handle to the completion queue of this ring.
        ALWAYS_INLINE auto GetCompletionQueue() -> CompletionQueue& {
            return m_completion_queue;
        }

    private:
        auto RegisterRaw(unsigned opcode, const void *arg, unsigned nargs) const -> int;

    public:
        /// Registers a sparse set of direct descriptors which can
        /// be used in operations instead of regular fds.
        auto RegisterFilesSparse(unsigned nfiles) const -> int;

        /// Removes all previously allocated direct descriptors
        /// from the ring.
        auto UnregisterFiles() const -> int;

        /// Enables a ring instance from disabled creation state.
        auto Enable() const -> int;

        /// Registers the file descriptor as a direct descriptor.
        /// This reduces the overhead on all io_uring system calls.
        auto RegisterRingFd() -> int;

        /// Unregisters the ring file descriptor from the direct
        /// descriptors set.
        auto UnregisterRingFd() -> int;

    private:
        auto Enter(unsigned want, __kernel_timespec *ts) const -> int;

    public:
        /// Submits pending submissions to the kernel without waiting for
        /// completions. Returns the number of submissions on success.
        auto Submit() const -> int;

        /// Submits pending submissions to the kernel and waits for want
        /// completions. An optional timeout can be provided for the wait.
        auto SubmitAndWait(unsigned want, std::optional<std::chrono::milliseconds> timeout = std::nullopt) const -> int;
    };

}
