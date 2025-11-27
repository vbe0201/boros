import pytest
from boros._low_level import *


def make_loop(sq_size: int, cq_size: int):
    p = EventLoopPolicy()
    p.sq_entries = sq_size
    p.cq_entries = cq_size

    create_event_loop(p)
    destroy_event_loop()


def test_sq_clamping():
    # Submission queue can't be zero-sized.
    with pytest.raises(OSError):
        make_loop(0, 0)

    # Make sure queue sizes are adjusted to fit the requirements.
    make_loop(1, 0)
    make_loop(2, 1)
    make_loop(2, 2)
    make_loop(5, 1)
    make_loop(16, 8)
    make_loop(100, 100)
    make_loop(256, 0)


# TODO: Test submissions when API is stable.
