# Type stubs for the native boros._low_level module.

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
