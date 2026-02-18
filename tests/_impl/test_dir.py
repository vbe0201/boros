import os
import tempfile

from boros import _impl
from .conftest import run, AT_REMOVEDIR, STATX_BASIC_STATS


class TestDirOps:
    def test_mkdirat_and_unlinkat(self, cfg):
        tmp = tempfile.mkdtemp()
        dirpath = os.path.join(tmp, "newdir")

        async def go():
            await _impl.mkdirat(None, dirpath, 0o755)
            assert os.path.isdir(dirpath)
            await _impl.unlinkat(None, dirpath, AT_REMOVEDIR)
            assert not os.path.exists(dirpath)

        run(cfg, go())
        os.rmdir(tmp)

    def test_symlinkat_and_readback(self, cfg):
        tmp = tempfile.mkdtemp()
        target = os.path.join(tmp, "target.txt")
        link = os.path.join(tmp, "link.txt")

        # Create the target file
        with open(target, "w") as f:
            f.write("target")

        async def go():
            await _impl.symlinkat(target, None, link)

        run(cfg, go())
        assert os.path.islink(link)
        assert os.readlink(link) == target

        os.unlink(link)
        os.unlink(target)
        os.rmdir(tmp)

    def test_linkat(self, cfg):
        tmp = tempfile.mkdtemp()
        original = os.path.join(tmp, "original.txt")
        hardlink = os.path.join(tmp, "hardlink.txt")

        with open(original, "w") as f:
            f.write("original")

        async def go():
            await _impl.linkat(None, original, None, hardlink, 0)

            # Both should have the same inode
            r1 = await _impl.statx(None, original, 0, STATX_BASIC_STATS)
            r2 = await _impl.statx(None, hardlink, 0, STATX_BASIC_STATS)
            assert r1.ino == r2.ino
            assert r1.nlink >= 2
            assert r2.nlink >= 2

        run(cfg, go())

        os.unlink(hardlink)
        os.unlink(original)
        os.rmdir(tmp)

    def test_unlinkat_file(self, cfg):
        tmp = tempfile.mkdtemp()
        path = os.path.join(tmp, "to_delete.txt")

        with open(path, "w") as f:
            f.write("delete me")

        async def go():
            await _impl.unlinkat(None, path, 0)

        run(cfg, go())
        assert not os.path.exists(path)
        os.rmdir(tmp)
