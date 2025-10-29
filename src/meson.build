# Disable exceptions and RTTI for all project targets.
# This reduces binary size and we don't want to raise
# exceptions on the C++ side anyway.
add_project_arguments(
    '-fno-exceptions',
    '-fno-rtti',
    '-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables',
    language: 'cpp',
)

subdir('boros')
subdir('impl')
