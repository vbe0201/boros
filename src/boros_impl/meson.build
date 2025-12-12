boros_impl_sources = [
    'module.c',
    'task.c',
    'util.c',

    'op/base.c',
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
