# Type stubs for the native boros._low_level module.

from typing import Any, Coroutine


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


class EventLoopPolicy:
    sq_entries: int
    cq_entries: int
    wqfd: int


class EventLoop:
    def tick(self):
        ...


def create_event_loop(policy: EventLoopPolicy) -> EventLoop:
    ...


def get_event_loop() -> EventLoop:
    ...


def destroy_event_loop():
    ...
