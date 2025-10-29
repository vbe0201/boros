// This source file is part of the boros project.
// SPDX-License-Identifier: ISC

#pragma once

#include "io_uring/cqueue.hpp"
#include "io_uring/squeue.hpp"
#include "macros.hpp"

namespace boros {

    /// A driver for an io_uring instance.
    class IoRing {
    private:
        SubmissionQueue m_submission_queue;
        CompletionQueue m_completion_queue;
        unsigned m_flags = 0;
        unsigned m_features = 0;
        int m_ring_fd = -1;
        bool m_registered = false;
        int m_enter_fd = -1;
        Mmap m_scq_mmap;
        Mmap m_sqe_mmap;

    public:
        IoRing() noexcept = default;
        ~IoRing() noexcept;

        /// Creates a new io_uring instance with the given queue size
        /// and configuration parameters and maps it to this instance.
        auto Initialize(unsigned entries, io_uring_params &p) noexcept -> int;

        /// Creates a new io_uring instance from a file descriptor
        /// and the configuration parameters.
        auto InitializeWithFile(int fd, io_uring_params &p) noexcept -> int;

        /// Tears down this io_uring instance. It can be recreated
        /// by another call to Initialize.
        auto Finalize() noexcept -> void;

        /// Indicates if an initialized io_uring is currently mapped
        /// to this instance.
        ALWAYS_INLINE auto IsInitialized() const noexcept -> bool {
            return m_scq_mmap.IsMapped();
        }

        /// Gets the configuration flags of the ring.
        ALWAYS_INLINE auto GetFlags() const noexcept -> unsigned {
            return m_flags;
        }

        /// Gets the supported io_uring feature flags.
        ALWAYS_INLINE auto GetFeatures() const noexcept -> unsigned {
            return m_features;
        }

        /// Gets the file descriptor associated with the io_uring,
        /// or -1 if it was closed already.
        ALWAYS_INLINE auto GetRingFd() const noexcept -> int {
            return m_ring_fd;
        }

        /// Gets a handle to the submission queue of this ring.
        ALWAYS_INLINE auto GetSubmissionQueue() noexcept -> SubmissionQueue& {
            return m_submission_queue;
        }

        /// Gets a handle to the completion queue of this ring.
        ALWAYS_INLINE auto GetCompletionQueue() noexcept -> CompletionQueue& {
            return m_completion_queue;
        }

    private:
        auto RegisterRaw(unsigned opcode, const void *arg, unsigned nargs) const noexcept -> int;

    public:
        /// Registers a sparse set of direct descriptors which
        /// can then be used over regular file descriptors.
        auto RegisterFilesSparse(unsigned nfiles) const noexcept -> int;

        /// Removes all previously allocated direct descriptors
        /// from the ring.
        auto UnregisterFiles() const noexcept -> int;

        /// Registers an io_uring_probe instance with the ring
        /// to test for supported opcodes. Note that the probe
        /// object must be allocated with capacity for 256
        /// io_uring_probe_op elements.
        auto RegisterProbe(io_uring_probe *probe) const noexcept -> int;

        /// Enables a ring instance which is in disabled state.
        auto Enable() const noexcept -> int;

        /// Registers the ring file descriptor as a direct
        /// descriptor. This reduces the overhead on all
        /// io_uring system calls.
        auto RegisterRingFd() noexcept -> int;

        /// Unregisters the ring file descriptor from the direct
        /// descriptors set.
        auto UnregisterRingFd() noexcept -> int;

        /// Closes the ring file descriptor. This can only be done
        /// after it was registered as a direct descriptor.
        auto CloseRingFd() noexcept -> int;

        /// Registers a buffer ring with this instance. Operations
        /// can then borrow memory from the buffer ring.
        auto RegisterBufferRing(io_uring_buf_reg *reg) const noexcept -> int;

        /// Unregisters a previously registered buffer ring from
        /// this instance.
        auto UnregisterBufferRing(int bgid) const noexcept -> int;

    public:
        /// Submits pending events to the kernel and waits until
        /// want completions can be reaped from the completion
        /// queue. Returns the number of submitted entries.
        auto SubmitAndWait(unsigned want) const noexcept -> int;
    };

}
