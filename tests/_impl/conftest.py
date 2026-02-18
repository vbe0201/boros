import pytest

from boros import _impl

AT_REMOVEDIR = 0x200
STATX_BASIC_STATS = 0x07FF


@pytest.fixture
def cfg():
    c = _impl.RunConfig()
    c.sq_size = 16
    return c


def run(cfg, coro):
    return _impl.run(coro, cfg)
