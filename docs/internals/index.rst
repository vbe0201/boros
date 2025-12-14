.. _internals:

Internals
=========

This section of the documentation is dedicated to the overall design
and implementation of boros. It provides a comprehensive overview of
all relevant mechanics, architectural choices, and guidelines for
navigating the codebase.

The target audience are boros developers and people interested in
the inner workings of I/O libraries or io_uring. As a library user,
you may skip this unless you're curious.

.. warning::

   The following chapters document implementation details which are
   subject to changes over time. The goal is to build a resource that
   provides guide-level explanations for reading and understanding
   the codebase and its design decisions in its current form.


.. toctree::
   :maxdepth: 2

   architecture
   io_with_io_uring
   task_management
