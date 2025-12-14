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
    ],
)

if host_machine.system() != 'linux'
    error('boros can only be built for Linux targets')
endif

liburing = dependency(
    'liburing',
    fallback: 'liburing',
    default_options: ['default_library=static'],
    required: true,
)

py = import('python').find_installation(pure: false)

subdir('src')
