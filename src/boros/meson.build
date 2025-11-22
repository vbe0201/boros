boros_sources = [
    '__init__.py',
    '_low_level.pyi',
    'py.typed',
]

boros_low_level_sources = [
    '_low_level/extension.cpp',
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
    install: true,
    subdir: 'boros',
    limited_api: '3.12',
)
