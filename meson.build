project(
    'boros',
    'cpp',
    license: 'ISC',
    meson_version: '>= 1.5.0',
    default_options: [
        'buildtype=debugoptimized',
        'warning_level=3',
        'cpp_std=c++23',
    ],
)

if host_machine.system() != 'linux'
    error('boros can only be built for Linux targets')
endif

py = import('python').find_installation(pure: false)

subdir('src')
