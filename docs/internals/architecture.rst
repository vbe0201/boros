.. _internals_architecture:

Architecture
============

This chapter explores the architecture of the project and sheds some
light on its internal composition.

The foundational building block for the library is the ``boros._impl``
module. It provides a self-contained implementation of the core scheduling
logic and the I/O framework in C. The module itself is not part of the
public API, but parts of it may be re-exported by the ``boros`` module.
No stability guarantees are made about this API.

On top of this layer, a pure-Python implementation of ``boros`` provides
useful abstractions and high-level features that users can build their
applications on.

.. _internals_runtime_flavors:

Runtime flavors
---------------

For the time being, ``boros`` focuses on providing a single-threaded
runtime flavor.

This means every thread has its own resources, event loop, tasks,
disjoint from other threads in the application. This is a natural
pattern that Python already provides through its :mod:`asyncio`
module.

With `free threading <https://docs.python.org/3/howto/free-threading-python.html>`_
in the works, wee will eventually want to support a second flavor
of runtime with a scheduler that leverages true hardware parallelism.
That is not a priority at the current time though and will not have
an influence on the public API of the ``boros`` module.
