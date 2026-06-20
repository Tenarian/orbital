include_dirs = include_directories('../lib/include', 'src')

qt6_dep = dependency('qt6', modules: ['Core', 'Network', 'DBus', 'Test'])

# Generate MOC files for GUI headers
gui_moc_sources = qt6_mod.compile_moc(
    headers: [
        'src/bridge/envvarmodel.h',
        'src/bridge/librarymodel.h',
        'src/bridge/protonmodel.h',
    ],
    include_directories: include_dirs,
    dependencies: [liborbital_dep, qt6_dep],
)

executable(
    'orbital-gui',
    'src/bridge/envvarmodel.cpp',
    'src/bridge/librarymodel.cpp',
    'src/bridge/protonmodel.cpp',
    'src/main.cpp',
    'src/qmlregister.cpp',
    gui_moc_sources,
    include_directories: include_dirs,
    dependencies: [
        liborbital_dep,
        qt6_dep,
        dependency('qt6', modules: ['DBus', 'Gui', 'Qml', 'Widgets']),
    ],
    install: true,
)
