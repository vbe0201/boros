import os
import struct
import socket

import pytest

from boros import _impl
from .conftest import run


class TestSocketOps:
    def test_socket_create_close(self, cfg):
        async def go():
            fd = await _impl.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
            assert fd >= 0
            await _impl.close(fd)

        run(cfg, go())

    def test_tcp_echo(self, cfg):
        async def go():
            srv = await _impl.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
            await _impl.setsockopt(srv, socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            await _impl.bind(srv, socket.AF_INET, ("127.0.0.1", 0))

            # The io_uring getsockname operation is too recent, so
            # we get the bound port through the stdlib instead.
            s = socket.socket(fileno=os.dup(srv))
            port = s.getsockname()[1]
            s.detach()

            await _impl.listen(srv, 5)

            cli = await _impl.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
            await _impl.connect(cli, socket.AF_INET, ("127.0.0.1", port))

            acc_fd, addr = await _impl.accept(srv, 0)
            assert acc_fd >= 0

            data = b"ping"
            sent = await _impl.send(cli, data, 0)
            assert sent == len(data)

            received = await _impl.recv(acc_fd, 1024, 0)
            assert received == data

            await _impl.close(acc_fd)
            await _impl.close(cli)
            await _impl.close(srv)

        run(cfg, go())

    def test_socket_wrong_arg_count(self):
        with pytest.raises(TypeError):
            _impl.socket(1)  # type: ignore[missing-argument]


class TestSockopt:
    def test_setsockopt_int_getsockopt_int(self, cfg):
        async def go():
            fd = await _impl.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
            await _impl.setsockopt(fd, socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            val = await _impl.getsockopt(fd, socket.SOL_SOCKET, socket.SO_REUSEADDR)
            assert val != 0
            await _impl.close(fd)

        run(cfg, go())

    def test_getsockopt_raw_bytes(self, cfg):
        async def go():
            fd = await _impl.socket(socket.AF_INET, socket.SOCK_STREAM, 0)

            raw = await _impl.getsockopt(fd, socket.SOL_SOCKET, socket.SO_TYPE, 4)
            assert isinstance(raw, bytes)
            assert len(raw) == 4

            val = struct.unpack("i", raw)[0]
            assert val == socket.SOCK_STREAM
            await _impl.close(fd)

        run(cfg, go())

    def test_setsockopt_bytes(self, cfg):
        async def go():
            fd = await _impl.socket(socket.AF_INET, socket.SOCK_STREAM, 0)

            await _impl.setsockopt(
                fd,
                socket.SOL_SOCKET,
                socket.SO_REUSEADDR,
                struct.pack("i", 1),
            )
            val = await _impl.getsockopt(fd, socket.SOL_SOCKET, socket.SO_REUSEADDR)
            assert val != 0
            await _impl.close(fd)

        run(cfg, go())

    def test_setsockopt_bad_value_type(self):
        with pytest.raises(TypeError):
            _impl.setsockopt(0, socket.SOL_SOCKET, socket.SO_REUSEADDR, [1, 2])  # type: ignore[no-matching-overload]

    def test_getsockopt_wrong_arg_count(self):
        with pytest.raises(TypeError):
            _impl.getsockopt(0, socket.SOL_SOCKET)  # type: ignore[no-matching-overload]
