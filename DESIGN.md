# Boros Design

This document explores some of the design considerations for Boros, a Python runtime for
asynchronous I/O.

## `asyncio`?

This project attempts to build a library that leverages the niceties of `io_uring` into a
fresh, user-friendly `async` framework.

While `asyncio` integrates with Windows and UNIX operating systems, `boros` intentionally
only supports recent Linux platforms. This allows for an API that can adopt all the latest
and greatest features to offer high performance out of the box.

Lastly, the `asyncio` API is riddled with redundant parts and design decisions that make parts
of it rather unwieldy at times. And at other times, important components like asynchronous file
I/O aren't featured at all. A popular third-party library to fill this gap is `aiofiles`. `boros`
tries to provide straightforward and hard-to-misuse APIs that do the right thing and cover all
important use cases.

## `io_uring`?

This is the Linux kernel API that `boros` is based on. People are looking at it mainly for async
disk I/O right now, but eventually it's going to be the future of all I/O on Linux.

The way it works is that `boros` and the Linux kernel communicate via two ring buffers in shared
memory - a Submission Queue for submitting I/O operations to the kernel and a Completion Queue
for receiving the result of the operation from the kernel when it's done. The actual processing
happens in the kernel, completely separated from our application.

Submitting to the Submission Queue and awaiting its result in the Completion Queue is unified
into `await`ing an operation.

## Lazy tasks

One of my personal gripes with `asyncio` is that you can end up with functions returning
immediately but still doing work in the background. Forgetting an `await` can be fatal.

This is not an issue in `boros`. Every task representing an async computation is lazy and
will do nothing unless `await`ed.

## Background tasks

There is no global `spawn` function to run work in the background. All background tasks
run in an associated task group:

```python
async with boros.task_group() as tg:
    tg.spawn(a)
    tg.spawn(b)
```

This has, in fact, multiple advantages:

- Requiring `async with` prevents misplaced calls to a `spawn` function outside of the
  `boros` execution context.

- At the end of a task group, all pending tasks are "joined", i.e. when a task group
  exits, every one of its tasks will be terminated.

- If one task raises an uncaught exception, all other tasks in the group will be cancelled.

- All unhandled exceptions at the end of the task group are re-raised in the parent task
  as an [`ExceptionGroup`](https://docs.python.org/3/library/exceptions.html#ExceptionGroup).

## Cancellation and Timeouts

Cancellation and Timeouts work with cancellation scopes:

```python
async with boros.cancel_after(10):  # Cancel after 10 seconds
    await boros.sleep(20)
```

Basically, most APIs will not concern themselves with cancellation directly. Instead, methods
in `boros` will detect when they're inside a cancellation scope and act accordingly. In the
same spirit, tasks inside a task group will inherit the cancellation scopes active for the
group itself. These scopes can be arbitrarily nested and a `boros.shield()` scope prevents
tasks from being cancellable.

This is especially useful for library code because in most cases, cancellation can be entirely
left to the user.

A cancellation is then realized by a `boros.Cancelled` exception being thrown. This exception
inherits from `BaseException` so it cannot be caught by `except Exception:`.

## Async Resources

`boros` models async resources as subclasses of the `AsyncDescriptor` type.

This type wraps the internal handle of the resource (either a fd or a registered handle), and
provides an async context manager for correctly closing it. This process involves cancelling
all pending operations on that file descriptor before actually awaiting an asynchronous close
operation.

Otherwise, this can lead to very nasty conditions where a descriptor with some outstanding
operations is closed, then re-assigned to something else, and then the operation completes,
resulting in very unpredictable behavior.

## Buffer Management

Buffer Management in `io_uring` is a tricky source of errors. Suppose you have some data, then
run an asynchronous operation on it. Now both the kernel and the user's code have concurrent,
unsynchronized access to this buffer. To prevent all types of nastiness, two constraints must
be fulfilled:

- Any buffers in use by an asynchronous operation may not be destroyed before the operation
  completes and must stay pinned to their memory location.

- Any buffers in use by an asynchronous operation may not be accessed for reading and writing
  by the user program, depending on the operation.

The first issue is solved by bumping the reference count of occupied resources for the duration
of an asynchronous operation. Address stability is a given since Python's `id()` builtin relies
on address stability as well.

The second issue is a lot more complicated because we cannot restrict how the user interacts
with the data it passed to the kernel. For `send`-like operations, we only accept immutable
types (`bytes`, `memoryview`) that the user cannot modify in good faith. For `recv`-like
operations we create our own buffers and only hand them to the user after an operation
completed. This is how most Python APIs work naturally, so no issues with that.

> TODO: Ideally users can't even read an occupied buffer. Can we do something?

## Buffer Pooling

As an optimization, users can pre-register a fixed set of buffers for use in I/O operations.

These buffers live in shared memory and can be directly read and written by the kernel without
the need for a memory copy between kernel and userspace. They are naturally a great fit for
imposing a memory limit on running programs and lend themselves to high performance I/O.

## Registered Handles

Another noteworthy optimization is the use of registered handles instead of file descriptors in
single-threaded programs.

Similar to buffers, you register a fixed-size table of file descriptors in advance and then you
can use these instead of normal file descriptors practically everywhere. This avoids the entire
`fget`/`fput` overhead associated with files in the kernel, even for the ring descriptor itself.

## Scheduling

The Submission Queue has a single-producer characteristic that optimizes for a single thread to
do all the submissions. This integrates well with Python's natural single-threaded model that
is also employed by other libraries like `asyncio`.

In a [Free-Threading World](https://py-free-threading.github.io) however, this does not leverage
the resources of a system efficiently. Users can choose to run more event loops on dedicated
threads. These loops should operate isolated from each other, ideally while trying to maintain
equal load across all workers.

One way to achieve this with a TCP server would be to submit accept operations on the same listener
in every worker thread, while ensuring overloaded workers back off. Defining runtime load is left
to the application, though pseudocode follows for 16 threads:

```python
async def handle_client(client: TcpStream):
    ...

async def worker(server: TcpListener):
    async with boros.task_group() as tg:
        while True:
            # Make sure at least one worker always has capacity
            # to avoid total starvation.
            await wait_for_capacity()

            # Accept a new connection and process it in the background.
            client = await server.accept()
            tg.spawn(handle_client, client)

async def main():
    # Set up a TCP server.
    server = await TcpListener.bind(...)

    # Spawn 15 worker threads and run one worker on this core as well.
    with ThreadPoolExecutor(max_workers=15) as ex:
        for _ in range(15):
            ex.submit(lambda: boros.run(worker, server))
    await worker(server)

boros.run(main)
```
