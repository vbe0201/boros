boros_cpp_sources = [
    'cqueue.cpp',
    'module.cpp',
    'operation.cpp',
    'python_utils.cpp',
    'ring.cpp',
    'runtime.cpp',
    'squeue.cpp',
    'task.cpp',
    'utils.cpp',
]

boros_cpp_args = [
    # This will no longer be required when we drop Python 3.12.
    '-DPY_SSIZE_T_CLEAN',

    # Disable C++ exceptions and RTTI to reduce binary size.
    '-fno-exceptions',
    '-fno-rtti',
    '-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables',
]

ext = py.extension_module(
    '_impl',
    boros_cpp_sources,
    include_directories: include_directories('.'),
    install: true,
    subdir: 'boros',
    limited_api: '3.12',
    cpp_args: boros_cpp_args,
)
