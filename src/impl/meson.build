ext = py.extension_module(
    '_impl',
    files(
        'cqueue.cpp',
        'module.cpp',
        'ring.cpp',
        'runtime.cpp',
        'squeue.cpp',
        'task.cpp',
        'utils.cpp',
    ),
    include_directories: include_directories('.'),
    install: true,
    subdir: 'boros',
    limited_api: '3.12',
    cpp_args : [
        '-DPY_SSIZE_T_CLEAN',
        '-fno-exceptions',
        '-fno-rtti',
        '-fno-unwind-tables',
        '-fno-asynchronous-unwind-tables',
    ],
)
