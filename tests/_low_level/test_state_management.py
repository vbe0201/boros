from threading import Thread

import pytest
from boros._low_level import *


def test_cannot_instantiate_event_loop():
    # This isn't possible because users must go through the global
    # state management functions to obtain event loop instances.
    with pytest.raises(TypeError):
        EventLoop()


def test_basic_state_management():
    policy = EventLoopPolicy()
    policy.sq_entries = 2

    # No loop is active yet, so should raise.
    with pytest.raises(RuntimeError):
        get_event_loop()

    # Should be a no-op when no loop is active.
    destroy_event_loop()

    # Still no active loop.
    with pytest.raises(RuntimeError):
        get_event_loop()

    # Try to create a loop with something other than a policy.
    with pytest.raises(TypeError):
        loop = create_event_loop(None)

    # Create the event loop for real.
    loop = create_event_loop(policy)

    # The instance should be the same as the cached one.
    assert loop is get_event_loop()

    # Now destroy the event loop.
    destroy_event_loop()

    # ...which should get us back to no active event loop configured.
    with pytest.raises(RuntimeError):
        get_event_loop()


def test_thread_local_loop_distinction():
    policy = EventLoopPolicy()
    policy.sq_entries = 2

    def create_and_destroy_loop(policy, out, idx):
        create_event_loop(policy)
        out[idx] = get_event_loop()
        destroy_event_loop()

    # We want to gather the event loops returned by get_event_loop() on
    # distinct threads so we can compare them and ensure they're not
    # the same object.
    loops = [None, None]
    t1 = Thread(target=create_and_destroy_loop, args=(policy, loops, 0))
    t2 = Thread(target=create_and_destroy_loop, args=(policy, loops, 1))

    t1.start()
    t2.start()

    t1.join()
    t2.join()

    assert loops[0] is not loops[1]
