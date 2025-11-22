# Disable exceptions and RTTI for all project targets.
# This reduces binary size and we want to report errors
# only to the Python side anyway.
add_project_arguments(
    '-fno-exceptions',
    '-fno-rtti',
    '-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables',
    language: 'cpp',
)

subdir('boros')
