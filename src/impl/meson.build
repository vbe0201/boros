boros_cpp_sources = [
    'io_uring/cqueue.cpp',
    'io_uring/mmap.cpp',
    'io_uring/ring.cpp',
    'io_uring/squeue.cpp',

    'wrapper/weakref.cpp',

    'implmodule.cpp',
    'runtime.cpp',
    'task.cpp',
]

ext = py.extension_module(
    '_impl',
    boros_cpp_sources,
    include_directories: include_directories('.'),
    install: true,
    subdir: 'boros',
    limited_api: '3.12',
)
