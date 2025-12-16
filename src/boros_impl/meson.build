boros_impl_sources = [
    'module.c',
    'run.c',
    'task.c',

    'context/run_config.c',
    'context/state.c',

    'io/proactor.c',

    'op/base.c',
    'op/nop.c',

    'util/outcome.c',
    'util/python.c',
]

py.extension_module(
    '_impl',
    boros_impl_sources,
    include_directories: include_directories('.'),
    dependencies: [liburing],
    install: true,
    subdir: 'boros',
)
