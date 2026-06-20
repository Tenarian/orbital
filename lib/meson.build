include_dirs = include_directories('include')

qt6_dep = dependency('qt6', modules: ['Core', 'Network', 'DBus', 'Test'])

# Generate MOC files for headers with Q_OBJECT
moc_sources = qt6_mod.compile_moc(
    headers: [
        'include/orbital/dbusinterface.h',
        'include/orbital/envvarschema.h',
        'include/orbital/gamelibrary.h',
        'include/orbital/prefixmanager.h',
        'include/orbital/presetmanager.h',
        'include/orbital/processlauncher.h',
        'include/orbital/protondbclient.h',
        'include/orbital/protonmanager.h',
    ],
    include_directories: include_dirs,
    dependencies: [qt6_dep],
)

liborbital = library(
    'liborbital',
    'src/dbusinterface.cpp',
    'src/envvarschema.cpp',
    'src/gamelibrary.cpp',
    'src/paths.cpp',
    'src/prefixmanager.cpp',
    'src/presetmanager.cpp',
    'src/processlauncher.cpp',
    'src/protondbclient.cpp',
    'src/protonmanager.cpp',
    moc_sources,
    include_directories: include_dirs,
    dependencies: [qt6_dep, dependency('qt6', modules: ['DBus'])],
    install: true,
)

liborbital_dep = declare_dependency(
    link_with: liborbital,
    include_directories: include_dirs,
    dependencies: [dependency('qt6', modules: ['DBus'])],
)
