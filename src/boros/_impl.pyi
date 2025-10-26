# Type stubs for the native boros._impl module.

from typing import Self


class Task:
    ...


class RuntimeContext:
    """
    The context of the currently active runtime instance.

    Must be explicitly set up with :meth:`enter` and can then be retrieved
    subsequently with :meth:`get`. Do not use the constructor to create
    instances of this type.

    The runtime is responsible for driving all I/O and timers, and scheduling
    suspended coroutines when they become ready. This allows Python code to
    submit operations, and query information about the runtime itself.

    Note that a runtime is optimized for single-threaded access only -- every
    thread may only operate on its own dedicated instance. It is recommended
    to always fetch the current runtime rather than caching a context value
    to avoid instance confusion between threads.
    """

    @classmethod
    def enter(cls, sq_entries: int, cq_entries: int, wq_fd: int) -> Self:
        """
        Enters the runtime context in the current thread.

        This will create a new io_uring instance for the current thread with
        the given parameters. Note that the runtime initially starts in a
        disabled state until it is manually enabled.

        :param sq_entries: The number of entries in the submission queue.
        :param cq_entries: The number of entries in the completion queue. Must
                           be a power of two and greater than sq_entries. When
                           set to 0, defaults to ``sq_entries``.
        :param wq_fd: The file descriptor of an existing ring whose backend in
                      the kernel should be shared with this ring. When set to
                      -1, a new backend will be created for this ring.
        :raises OSError: Invalid arguments were supplied, or the kernel could
                         not create an io_uring instance.
        :returns: The newly established RuntimeContext instance.
        :raises RuntimeError: A runtime is already active in the current thread.
        """
        ...

    @staticmethod
    def exit():
        """
        Exits the runtime context on the current thread.
        
        This is a no-op if no runtime context is established. :meth:`enter` can
        be called again afterwards to establish a new context anytime.
        """
        ...

    @classmethod
    def get(cls) -> Self:
        """
        Gets a handle to the current :class:`RuntimeContext`.

        Since the backing io_uring instances can only be driven from the
        thread that created them, this provides a convenient handle to
        the correct instance from where this method is called.

        :returns: A :class:`RuntimeContext` instance.
        :raises RuntimeError: When called outside a runtime execution environment.
        """
        ...

    def get_ring_fd(self) -> int:
        """
        Gets the file descriptor of the associated io_uring instance.

        :returns: The file descriptor, or -1 if it was closed.
        """
        ...

    def enable_ring(self):
        """
        Enables the associated io_uring instance.

        This must be done after calling :meth:`enter` before any I/O operations
        can be submitted.

        :raises OSError: Failed to enable the ring in the kernel.
        """
        ...
