boros_impl_sources = [
    'module.c',
    'run.c',
    'task.c',

    'driver/handle.c',
    'driver/proactor.c',
    'driver/run_config.c',

    'op/base.c',
    'op/cancel.c',
    'op/close.c',
    'op/connect.c',
    'op/fsync.c',
    'op/linkat.c',
    'op/mkdir.c',
    'op/nop.c',
    'op/open.c',
    'op/read.c',
    'op/rename.c',
    'op/socket.c',
    'op/symlinkat.c',
    'op/unlinkat.c',
    'op/write.c',

    'util/outcome.c',
    'util/python.c',
    'util/sockaddr.c',
]

py.extension_module(
    '_impl',
    boros_impl_sources,
    include_directories: include_directories('.'),
    dependencies: [liburing],
    install: true,
    subdir: 'boros',
)
