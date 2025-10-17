# Type stubs for the native boros._impl module.

from typing import Self


class RuntimeContext:
    """
    The context of the currently active runtime instance.

    The runtime is responsible for driving all I/O and timers, and scheduling
    suspended coroutines when they become ready. This allows Python code to
    submit operations, and query information about the runtime itself.

    Note that a runtime is designed for single-threaded access only -- every
    thread may only operate on its own dedicated runtime instance. It is
    recommended to always call :meth:`get` rather than caching the return
    value to avoid interfering with another thread's runtime by accident.
    """

    @classmethod
    def get(cls) -> Self:
        """
        Gets a handle to the current :class:`RuntimeContext`.

        Since the backing io_uring instances can only be driven from the
        thread that created them, this provides a convenient handle to
        the correct instance from where this method is called.

        :return: A :class:`RuntimeContext` instance.
        :raises RuntimeError: When called outside a runtime execution environment.
        """
        ...
