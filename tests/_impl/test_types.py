import os
import tempfile

import pytest

from boros import _impl
from .conftest import run, AT_REMOVEDIR, STATX_BASIC_STATS


class TestArgTypes:
    def test_int_from_bool(self, cfg):
        async def go():
            assert await _impl.nop(True) == 1
            assert await _impl.nop(False) == 0

        run(cfg, go())

    def test_none_as_dfd(self, cfg):
        tmp = tempfile.mkdtemp()

        async def go():
            p = os.path.join(tmp, "dfd_none.txt")
            fd = await _impl.openat(None, p, os.O_CREAT | os.O_WRONLY, 0o644)
            await _impl.close(fd)

            result = await _impl.statx(None, p, 0, STATX_BASIC_STATS)
            assert result.size == 0

            await _impl.unlinkat(None, p, 0)

            d = os.path.join(tmp, "dfd_none_dir")
            await _impl.mkdirat(None, d, 0o755)
            await _impl.unlinkat(None, d, AT_REMOVEDIR)

        run(cfg, go())
        os.rmdir(tmp)

    def test_path_from_pathlike(self, cfg):
        tmp = tempfile.mkdtemp()

        class MyPath:
            def __init__(self, p):
                self._p = p

            def __fspath__(self):
                return self._p

        async def go():
            p = MyPath(os.path.join(tmp, "pathlike.txt"))
            fd = await _impl.openat(None, p, os.O_CREAT | os.O_WRONLY, 0o644)
            await _impl.close(fd)

        run(cfg, go())
        os.unlink(os.path.join(tmp, "pathlike.txt"))
        os.rmdir(tmp)

    def test_float_rejected_as_int(self):
        with pytest.raises(TypeError):
            _impl.nop(1.5)  # type: ignore[invalid-argument-type]
