// This source file is part of the boros project.
// SPDX-License-Identifier: MIT

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

        /// Creates a new io_uring instance with the given number of submission
        /// queue entries and the configuration parameters.
        auto Create(unsigned entries, io_uring_params &p) noexcept -> int;

        /// Creates a new io_uring instance from a already set up file descriptor
        /// and the configuration parameters.
        auto CreateWithFile(int fd, io_uring_params &p) noexcept -> int;

        /// Gets the configuration flags of the ring.
        ALWAYS_INLINE auto GetFlags() const noexcept -> unsigned {
            return m_flags;
        }

        /// Gets the supported io_uring feature flags reported by the kernel.
        ALWAYS_INLINE auto GetFeatures() const noexcept -> unsigned {
            return m_features;
        }

        /// Gets a handle to the submission queue of this ring instance.
        ALWAYS_INLINE auto GetSubmissionQueue() noexcept -> SubmissionQueue& {
            return m_submission_queue;
        }

        /// Gets a handle to the completion queue of this ring instance.
        ALWAYS_INLINE auto GetCompletionQueue() noexcept -> CompletionQueue& {
            return m_completion_queue;
        }

    private:
        /// Performs a raw io_uring_register operation with the arguments.
        auto RegisterRaw(unsigned int opcode, const void *arg, unsigned int num_args) const noexcept -> int;

    public:
        /// Registers a sparse set of direct descriptors which can then be
        /// assigned and used instead of regular file descriptors.
        auto RegisterFilesSparse(unsigned num_files) const noexcept -> int;

        /// Removes all previously allocated direct descriptors from this
        /// ring instance.
        auto UnregisterFiles() const noexcept -> int;

        /// Registers an io_uring_probe instance with the ring to test for
        /// supported opcodes. Note that the probe object must be allocated
        /// with capacity for 256 io_uring_probe_op elements.
        auto RegisterProbe(io_uring_probe *probe) const noexcept -> int;

        /// Enables a ring instance in disabled state after having it created
        /// the IORING_SETUP_R_DISABLED configuration flag.
        auto Enable() const noexcept -> int;

        /// Registers the ring file descriptor itself as a direct descriptor.
        /// This can reduce the overhead of all io_uring syscalls.
        auto RegisterRingFd() noexcept -> int;

        /// Unregisters the ring file descriptor from the direct descriptors.
        auto UnregisterRingFd() noexcept -> int;

        /// Closes the ring file descriptor. This can only be done after the
        /// descriptor has been registered as a direct descriptor.
        auto CloseRingFd() noexcept -> int;

        /// Registers a buffer ring with this ring instance. This allows
        /// operations to borrow memory from the buffers in this ring.
        auto RegisterBufRing(io_uring_buf_reg *reg) const noexcept -> int;

        /// Unregisters a previously registered buffer ring instance from
        /// this ring instance.
        auto UnregisterBufRing(int bgid) const noexcept -> int;

        /// Obtains the current head of a previously registered buffer ring.
        auto GetBufRingHead(int buf_group, std::uint16_t *head) const noexcept -> int;

    public:
        /// Submits pending events from the submission queue to the kernel
        /// and waits until ``want`` completions can be reaped. Returns
        /// the number of submitted entries on success.
        auto SubmitAndWait(unsigned want) const noexcept -> int;
    };

}
