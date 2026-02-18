import pytest

from boros import _impl
from .conftest import run


class TestNop:
    def test_nop_echo(self, cfg):
        async def go():
            return await _impl.nop(42)

        assert run(cfg, go()) == 42

    def test_nop_negative(self, cfg):
        async def go():
            return await _impl.nop(-1)

        assert run(cfg, go()) == -1

    def test_nop_wrong_type(self):
        with pytest.raises((TypeError, SystemError)):
            _impl.nop("hello")  # type: ignore[invalid-argument-type]

    def test_nop_overflow(self):
        with pytest.raises(OverflowError):
            _impl.nop(2**64)
