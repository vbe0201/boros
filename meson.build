project(
    'boros',
    'c',
    license: 'ISC',
    meson_version: '>= 1.5.0',
    default_options: [
        'buildtype=debug',
        'warning_level=2',
        'c_std=gnu23',
        'b_ndebug=if-release',

        'liburing:default_library=static',
        'liburing:b_lto=false',
        'liburing:warning_level=0',
    ],
)

if host_machine.system() != 'linux'
    error('boros can only be built for Linux targets')
endif

liburing_source = get_option('liburing_source')
if liburing_source == 'system'
    liburing = dependency('liburing', version: '>= 2.12', static: true, required: true)
    message('Using system liburing (forced)')
elif liburing_source == 'bundled'
    liburing = subproject('liburing').get_variable('uring')
    message('Using bundled liburing (forced)')
else
    liburing = dependency(
        'liburing',
        version: '>= 2.12',
        fallback: 'liburing',
        static: true,
        required: true,
    )
endif

py = import('python').find_installation(pure: false)

subdir('boros')
subdir('src')
