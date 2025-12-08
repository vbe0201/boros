boros_sources = [
    '__init__.py',
    '_low_level.pyi',
    'py.typed',
]

boros_low_level_sources = [
    '_low_level/event_loop.cpp',
    '_low_level/extension.cpp',
    '_low_level/task.cpp',

    '_low_level/io/cqueue.cpp',
    '_low_level/io/mmap.cpp',
    '_low_level/io/ring.cpp',
    '_low_level/io/squeue.cpp',
]

py.install_sources(
    boros_sources,
    pure: false,
    subdir: 'boros',
)

py.extension_module(
    '_low_level',
    boros_low_level_sources,
    include_directories: include_directories('_low_level'),
    dependencies: [boros_binding_dep],
    install: true,
    subdir: 'boros',
    limited_api: '3.12',
)
