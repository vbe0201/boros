.. _internals_io_with_io_uring:

I/O with io_uring
=================

This chapter explores how we leverage `io_uring <https://man7.org/linux/man-pages/man7/io_uring.7.html>`_
for handling I/O in the runtime.

.. _internals_io_platform_support:

Platform support
----------------

``boros`` targets Linux kernels starting from version 6.1 and newer.
This is conservatively chosen based on the availability of features
we don't want to miss out on in core functionality.

With that being said, ``io_uring`` is still considered a somewhat
novel technology and kernel versions well beyond 6.1 continue to
add more system calls and other useful features to this day. For
the best experience, we always recommend a recent stable kernel
and we may bump our kernel version requirements in the future.

Ring setup
----------

An ``io_uring`` instance consists of a *Submission Queue* for I/O
operations made by the application and a *Completion Queue* for
reading back the results of the operations. All the I/O processing
is offloaded into the kernel by an ``io_uring_enter`` system call,
which allows for some nice tricks showcased in the following.

We keep one ring instance per application thread and set it up with
the ``IORING_SETUP_DEFER_TASKRUN | IORING_SETUP_SINGLE_ISSUER`` flags
to optimize for the single-producer interaction.

.. note::

   One might notice the ``IORING_SETUP_SQPOLL`` flag while browsing
   the man page. It spins up a kernel thread which polls your queue
   so you don't have to submit your operations. It is technically
   possible to write applications that do zero system calls with it.

   However, ``boros`` does not and will never support this mode of
   operation. It has the allure of speeding things up, but rarely
   does in practice and can cause excessive CPU usage. The fast path
   is to ``io_uring_enter`` with submissions and wait for their
   completion inline.

Batching
--------

One of the bigger opportunities with ``io_uring`` is that we are not
required to issue a system call per I/O operation.

Our event loop will be organized into three phases to take maximum
advantage of this:

* Drain the current run queue and resume all suspended tasks. Each of
  those tasks may then suspend on an I/O operation that gets added to
  the current thread's *Submission Queue*.

* When the queue is empty, submit all accumulated I/O to the kernel
  and wait for some completions since there's nothing to do in the
  meantime.

* Upon waking up, reap completions from the *Completion Queue* and
  add the woken tasks back to the run queue. The loop starts over
  again.

Files
-----

TODO

Buffers
-------

TODO
