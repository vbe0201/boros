import pytest

from boros import _impl
from .conftest import run


class TestRuntime:
    def test_run_requires_two_args(self):
        with pytest.raises(TypeError):
            _impl.run()  # type: ignore[missing-argument]

        async def noop():
            pass

        with pytest.raises(TypeError):
            _impl.run(noop())  # type: ignore[missing-argument]

    def test_run_requires_runconfig(self):
        async def noop():
            pass

        with pytest.raises(TypeError):
            _impl.run(noop(), "not a config")  # type: ignore[invalid-argument-type]

    def test_nested_runtime_forbidden(self, cfg):
        async def outer():
            async def inner():
                pass

            _impl.run(inner(), cfg)

        with pytest.raises(RuntimeError):
            run(cfg, outer())

    def test_bad_yield_value(self, cfg):
        class BadAwaitable:
            def __await__(self):
                yield "garbage"

        async def bad_coro():
            await BadAwaitable()

        with pytest.raises(RuntimeError):
            run(cfg, bad_coro())

    def test_operation_not_awaited_twice(self, cfg):
        async def double_await():
            op = _impl.nop(42)
            r1 = await op
            r2 = await op
            return r1, r2

        with pytest.raises(RuntimeError, match="Operation result was already consumed"):
            run(cfg, double_await())

    def test_sequential_runs(self, cfg):
        async def first():
            return await _impl.nop(1)

        async def second():
            return await _impl.nop(2)

        assert run(cfg, first()) == 1
        assert run(cfg, second()) == 2
