boros_impl_sources = [
    'module.c',
    'task.c',

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
    limited_api: '3.12',
)
