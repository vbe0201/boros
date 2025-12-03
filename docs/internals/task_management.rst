.. _internals_task_management:

Task management
===============

This chapter goes into detail on how we manage tasks efficiently.

Conceptually, our tasks resemble `green threads <https://en.wikipedia.org/wiki/Green_thread>`_
which can be found in other programming languages too. They can be
thought of as threads but with some key differences:

* Tasks are **lightweight**. They are scheduled by the application
  rather than the operating system, which makes them cheap to create,
  schedule, and destroy in large numbers.

* Tasks are **cooperative**. This means a task effectively runs until
  it decides it is time to suspend to give another task the chance to
  run, whereas in operating systems it's common to implement *preemptive*
  models where the scheduler can swap out a running task after some time.

* Tasks are **coroutine runners**. While a :class:`~threading.Thread`
  may run arbitrary code, a boros task always maps onto a *coroutine*
  and drives its execution. This allows us to introduce cooperative
  scheduling points with the ``await`` keyword.

We provide a user-facing :class:`~boros._low_level.Task` abstraction
on the Python side which exposes some metadata about a task in the
system, but is otherwise not relevant for the intricacies of task
handling.

.. _internals_task_lists:

Task lists
----------

In order to track tasks which are ready to make progress, we put them
into thread-local *run queues*.

These queues are doubly-linked lists which give us *O(1)* insertion and
removal characteristics. No synchronization is needed for accessing
them because each thread keeps its own local list.

And by nature of :class:`~boros._low_level.Task` already being an
allocated and garbage-collected Python object, we place the list
node directly into the object to avoid further memory allocations
for this data structure.

For scheduling this implies that a task will always remain pinned to
the thread it was created on.

.. _internals_task_locals:

Task locals
-----------

Similar to :class:`threading.local`, tasks may need a convenient
mechanism for storing and retrieving local per-task state.

We will provide strong integration with the :mod:`contextvars`
module out of the box. This will be done by giving each task a
copy of the current context state of the thread and temporarily
entering a task's state when it gets to run.
