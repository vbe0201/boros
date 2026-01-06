# Type stubs for the native boros._low_level module.

from collections.abc import Awaitable, Coroutine
from os import PathLike
from typing import Any, TypeVar

T = TypeVar("T")


class Task:
    """
    A lightweight, concurrent thread of execution.

    Tasks are similar to OS threads, but they are managed by the boros scheduler
    instead of the OS scheduler. This makes them cheap to create and there is
    little overhead to switching between tasks.

    :class:`Task` has no public constructor and appears immutable to Python code.
    Its public members are mostly useful for introspection and debugging.
    """

    @property
    def name(self) -> str:
        """A string representation of the task name."""
        ...

    @property
    def coro(self) -> Coroutine[Any, Any, Any]:
        """The coroutine object associated with the task."""
        ...


class RunConfig:
    """
    Configuration for the runtime context.

    These settings are used to create the subsystems and must be passed to
    every runner invocation.
    """

    #: The capacity of the io_uring submission queue.
    sq_entries: int
    #: The capacity of the io_uring completion queue.
    cq_entries: int
    #: The fd of an existing io_uring instance whose work queue should be shared.
    wqfd: int


def nop(echo: int) -> Awaitable[int]:
    """Asynchronous nop operation on the io_uring."""
    ...


def socket(domain: int, type: int, protocol: int) -> Awaitable[int]:
    """Asynchronous socket(2) operation on the io_uring."""
    ...


def read(fd: int, count: int, offset: int) -> Awaitable[bytes]:
    """Asynchronous read(2) operation on the io_uring."""
    ...


def write(fd: int, buf: bytes, offset: int) -> Awaitable[int]:
    """Asynchronous write(2) operation on the io_uring."""
    ...


def close(fd: int) -> Awaitable[int]:
    """Asynchronous close(2) operation on the io_uring."""
    ...


def open(
    path: str | bytes | PathLike[str] | PathLike[bytes],
    flags: int,
    mode: int,
) -> Awaitable[int]:
    """Asynchronous open(2) operation on the io_uring."""
    ...


def run(coro: Coroutine[Any, None, T], conf: RunConfig) -> T:
    """
    Drives a given coroutine to completion.

    This is the entrypoint to the boros runtime.
    """
    ...
