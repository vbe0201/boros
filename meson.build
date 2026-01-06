project(
    'boros',
    'c',
    license: 'ISC',
    meson_version: '>= 1.5.0',
    default_options: [
        'buildtype=debugoptimized',
        'warning_level=2',
        'c_std=gnu23',
        'b_ndebug=if-release',

        'liburing:default_library=static',
        'liburing:warning_level=0',
    ],
)

if host_machine.system() != 'linux'
    error('boros can only be built for Linux targets')
endif

liburing = dependency(
    'liburing',
    version: '>= 2.12',
    fallback: 'liburing',
    static: true,
    required: true,
)

py = import('python').find_installation(pure: false)

subdir('src')
