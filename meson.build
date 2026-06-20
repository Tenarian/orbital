project(
    'orbital',
    'cpp',
    version: '0.1.0',
    default_options: ['cpp_std=c++20', 'warning_level=2'],
)

# Suppress warnings from third-party headers that we cannot fix
# (GCC 16.1.1 + Qt6 SFINAE incomplete type warnings)
add_project_arguments('-Wno-sfinae-incomplete', language: 'cpp')

qt6_mod = import('qt6')

subdir('lib')
if get_option('cli')
    subdir('cli')
endif
if get_option('gui')
    subdir('gui')
endif
subdir('tests')

install_subdir('assets/schema', install_dir: get_option('datadir') / 'orbital' / 'schema')
