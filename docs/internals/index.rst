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

   None of the implementation details are set in stone. boros explores
   a relatively novel design space with its focus on io_uring, and if
   superior approaches are discovered they will be incorporated.


.. toctree::
   :maxdepth: 2

   architecture
   io_with_io_uring
   task_management
