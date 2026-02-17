# Type stubs for the native boros._impl module.

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

class StatxResult:
    """Result of a statx(2) system call."""

    #: Last access time in seconds since the epoch.
    atime: int
    #: Nanosecond part of the last access time.
    atime_nsec: int
    #: Preferred block size for I/O.
    blksize: int
    #: Number of 512-byte blocks allocated.
    blocks: int
    #: Last status change time in seconds since the epoch.
    ctime: int
    #: Nanosecond part of the last status change time.
    ctime_nsec: int
    #: Major device number of the device containing the file.
    dev_major: int
    #: Minor device number of the device containing the file.
    dev_minor: int
    #: Group ID of the file owner.
    gid: int
    #: Inode number.
    ino: int
    #: File type and permissions.
    mode: int
    #: Last modification time in seconds since the epoch.
    mtime: int
    #: Nanosecond part of the last modification time.
    mtime_nsec: int
    #: Number of hard links.
    nlink: int
    #: Major device number of the file, if it is a device.
    rdev_major: int
    #: Minor device number of the file, if it is a device.
    rdev_minor: int
    #: Total size in bytes.
    size: int
    #: User ID of the file owner.
    uid: int

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

def mkdirat(
    dfd: int,
    path: _PathT,
    mode: int,
) -> Awaitable[None]:
    """Asynchronous mkdir(2) operation on the io_uring."""
    ...

def renameat(
    olddfd: int,
    oldpath: _PathT,
    newdfd: int,
    newpath: _PathT,
    flags: int,
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
    ...

def symlinkat(
    target: _PathT,
    newdirfd: int | None,
    linkpath: _PathT,
) -> Awaitable[None]:
    """Asynchronous symlinkat(2) operation on the io_uring."""

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

def accept(fd: int, flags: int) -> Awaitable[tuple[int, _SockAddrT]]:
    """Asynchronous accept(2) operation on the io_uring."""
    ...

@overload
def bind(fd: int, af: Literal[AddressFamily.AF_INET], address: _SockAddrV4T) -> Awaitable[None]:
    ...

@overload
def bind(fd: int, af: Literal[AddressFamily.AF_INET6], address: _SockAddrV6T) -> Awaitable[None]:
    ...

@overload
def bind(fd: int, af: Literal[AddressFamily.AF_UNIX], address: _PathT) -> Awaitable[None]:
    ...

def bind(fd: int, af: int, address: _SockAddrT) -> Awaitable[None]:
    """Asynchronous bind(2) operation on the io_uring."""
    ...

def listen(fd: int, backlog: int) -> Awaitable[None]:
    """Asynchronous listen(2) operation on the io_uring."""
    ...

def send(fd: int, buf: bytes, flags: int) -> Awaitable[int]:
    """Asynchronous send(2) operation on the io_uring."""
    ...

def recv(fd: int, count: int, flags: int) -> Awaitable[bytes]:
    """Asynchronous recv(2) operation on the io_uring."""
    ...

def statx(dfd: int | None, path: _PathT, flags: int, mask: int) -> Awaitable[StatxResult]:
    """Asynchronous statx(2) operation on the io_uring."""
    ...

@overload
def getsockopt(fd: int, level: int, optname: int) -> Awaitable[int]:
    ...

@overload
def getsockopt(fd: int, level: int, optname: int, optlen: int) -> Awaitable[bytes]:
    ...

def getsockopt(fd: int, level: int, optname: int, optlen: int = ...) -> Awaitable[int | bytes]:
    """Asynchronous getsockopt(2) operation on the io_uring."""
    ...

@overload
def setsockopt(fd: int, level: int, optname: int, value: int) -> Awaitable[None]:
    ...

@overload
def setsockopt(fd: int, level: int, optname: int, value: bytes) -> Awaitable[None]:
    ...

def setsockopt(fd: int, level: int, optname: int, value: int | bytes) -> Awaitable[None]:
    """Asynchronous setsockopt(2) operation on the io_uring."""
    ...

def run(coro: Coroutine[Any, None, _RunT], conf: RunConfig) -> _RunT:
    """
    Drives a given coroutine to completion.

    This is the entrypoint to the boros runtime.
    """
    ...
