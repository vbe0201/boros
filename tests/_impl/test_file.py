import os
import pathlib
import tempfile

import pytest

from boros import _impl
from .conftest import run, STATX_BASIC_STATS


class TestFileOps:
    def test_open_read_write_close(self, cfg):
        tmp = tempfile.mkdtemp()
        path = os.path.join(tmp, "test.bin")

        async def go():
            fd = await _impl.openat(
                None, path, os.O_CREAT | os.O_WRONLY | os.O_TRUNC, 0o644
            )
            data = b"hello from boros!"
            n = await _impl.write(fd, data, 0)
            assert n == len(data)
            await _impl.close(fd)

            fd = await _impl.openat(None, path, os.O_RDONLY, 0)
            buf = await _impl.read(fd, 4096, 0)
            assert buf == data
            await _impl.close(fd)

        run(cfg, go())
        os.unlink(path)
        os.rmdir(tmp)

    def test_openat_dfd_none(self, cfg):
        tmp = tempfile.mkdtemp()
        path = os.path.join(tmp, "none_dfd.txt")

        async def go():
            fd = await _impl.openat(None, path, os.O_CREAT | os.O_WRONLY, 0o644)
            await _impl.close(fd)

        run(cfg, go())
        assert os.path.exists(path)
        os.unlink(path)
        os.rmdir(tmp)

    def test_openat_path_types(self, cfg):
        tmp = tempfile.mkdtemp()

        async def go():
            # str
            p1 = os.path.join(tmp, "str.txt")
            fd = await _impl.openat(None, p1, os.O_CREAT | os.O_WRONLY, 0o644)
            await _impl.close(fd)

            # bytes
            p2 = os.path.join(tmp, "bytes.txt").encode()
            fd = await _impl.openat(None, p2, os.O_CREAT | os.O_WRONLY, 0o644)
            await _impl.close(fd)

            # pathlib.Path
            p3 = pathlib.Path(tmp) / "path.txt"
            fd = await _impl.openat(None, p3, os.O_CREAT | os.O_WRONLY, 0o644)
            await _impl.close(fd)

        run(cfg, go())

        assert os.path.exists(os.path.join(tmp, "str.txt"))
        assert os.path.exists(os.path.join(tmp, "bytes.txt"))
        assert os.path.exists(os.path.join(tmp, "path.txt"))

        for name in ("str.txt", "bytes.txt", "path.txt"):
            os.unlink(os.path.join(tmp, name))
        os.rmdir(tmp)

    def test_openat_wrong_arg_count(self):
        with pytest.raises(TypeError):
            _impl.openat(None, "/tmp/x")  # type: ignore[missing-argument]  # too few

        with pytest.raises(TypeError):
            _impl.openat(None, "/tmp/x", 0, 0, 0)  # type: ignore[too-many-positional-arguments]  # too many

    def test_openat_bad_path_type(self):
        with pytest.raises(TypeError):
            _impl.openat(None, 12345, os.O_RDONLY, 0)  # type: ignore[invalid-argument-type]

    def test_read_bad_fd(self, cfg):
        async def go():
            await _impl.read(99999, 64, 0)

        with pytest.raises(OSError):
            run(cfg, go())

    def test_write_requires_bytes(self):
        with pytest.raises(TypeError):
            _impl.write(0, "string", 0)  # type: ignore[invalid-argument-type]

    def test_statx_basic(self, cfg):
        data = b"statx test data"
        tmp = tempfile.NamedTemporaryFile(delete=False)
        tmp.write(data)
        tmp.close()
        path = tmp.name

        try:

            async def go():
                result = await _impl.statx(None, path, 0, STATX_BASIC_STATS)
                assert result.size == len(data)
                assert result.uid == os.getuid()
                assert result.nlink >= 1
                return result

            run(cfg, go())
        finally:
            os.unlink(path)

    def test_statx_result_readonly(self, cfg):
        tmp = tempfile.NamedTemporaryFile(delete=False)
        tmp.write(b"x")
        tmp.close()
        path = tmp.name

        try:

            async def go():
                result = await _impl.statx(None, path, 0, STATX_BASIC_STATS)
                with pytest.raises(AttributeError):
                    result.size = 999

            run(cfg, go())
        finally:
            os.unlink(path)

    def test_fsync(self, cfg):
        tmp = tempfile.mkdtemp()
        path = os.path.join(tmp, "fsync.txt")

        async def go():
            fd = await _impl.openat(None, path, os.O_CREAT | os.O_WRONLY, 0o644)
            await _impl.write(fd, b"sync me", 0)
            await _impl.fsync(fd, 0)
            await _impl.close(fd)

        run(cfg, go())
        os.unlink(path)
        os.rmdir(tmp)
