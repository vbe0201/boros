# Type stubs for the native boros._low_level module.

from collections.abc import Awaitable, Coroutine
from os import PathLike
from socket import AddressFamily
from typing import Any, Literal, TypeAlias, TypeVar, overload

_RunT = TypeVar("_RunT")

_PathT: TypeAlias = str | bytes | PathLike[str] | PathLike[bytes]

_HostIpT: TypeAlias = str | bytes | bytearray
_SockAddrV4T: TypeAlias = tuple[_HostIpT, int]
_SockAddrV6T: TypeAlias = tuple[_HostIpT, int] | tuple[_HostIpT, int, int] | tuple[_HostIpT, int, int, int]
_SockAddrT: TypeAlias = _SockAddrV4T | _SockAddrV6T | _PathT

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
    sq_size: int
    #: The capacity of the io_uring completion queue.
    cq_size: int
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

def openat(dfd: int | None, path: _PathT, flags: int, mode: int) -> Awaitable[int]:
    """Asynchronous openat(2) operation on the io_uring."""
    ...

def cancel_fd(fd: int) -> Awaitable[int]:
    """Asynchronously cancels all operations on a fd."""
    ...

def cancel_op(op: Awaitable[Any]) -> Awaitable[int]:
    """Asynchronously cancels a specific operation."""
    ...

def mkdir(
    path: _PathT,
    mode: int,
) -> Awaitable[None]:
    """Asynchronous mkdir(2) operation on the io_uring."""
    ...

def rename(
    oldpath: _PathT,
    newpath: _PathT,
) -> Awaitable[None]:
    """Asynchronous rename(2) operation on the io_uring."""
    ...

def fsync(
    fd: int,
    flags: int,
) -> Awaitable[None]:
    """Asynchronous fsync(2) operation on the io_uring."""
    ...

def linkat(
    olddirfd: int | None,
    oldpath: _PathT,
    newdirfd: int | None,
    newpath: _PathT,
    flags: int,
) -> Awaitable[None]:
    """Asynchronous linkat(2) operation on the io_uring."""
    ...

def unlinkat(
    dirfd: int | None,
    path: _PathT,
    flags: int,
) -> Awaitable[None]:
    """Asynchronous unlinkat(2) operation on the io_uring."""

@overload
def connect(fd: int, af: Literal[AddressFamily.AF_INET], address: _SockAddrV4T) -> Awaitable[None]:
    ...


@overload
def connect(fd: int, af: Literal[AddressFamily.AF_INET6], address: _SockAddrV6T) -> Awaitable[None]:
    ...


@overload
def connect(fd: int, af: Literal[AddressFamily.AF_UNIX], address: _PathT) -> Awaitable[None]:
    ...


def connect(fd: int, af: int, address: _SockAddrT) -> Awaitable[None]:
    """Asynchronous connect(2) operation on the io_uring."""
    ...

def run(coro: Coroutine[Any, None, _RunT], conf: RunConfig) -> _RunT:
    """
    Drives a given coroutine to completion.

    This is the entrypoint to the boros runtime.
    """
    ...
