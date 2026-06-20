executable(
    'orbital',
    files(
        'src/commands/env.cpp',
        'src/commands/library.cpp',
        'src/commands/prefix.cpp',
        'src/commands/preset.cpp',
        'src/commands/proton.cpp',
        'src/main.cpp',
        'src/parser.cpp',
    ),
    include_directories: include_directories('../lib/include', '.', '../cli/src'),
    dependencies: [liborbital_dep, dependency('qt6', modules: ['DBus', 'Network'])],
    install: true,
)
