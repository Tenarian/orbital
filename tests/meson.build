# Test include directory
test_include = include_directories('include', '../lib/include')

# Tests need to find built-in schemas under assets/schema/. In test mode,
# QStandardPaths::locate(GenericDataLocation, "orbital/schema") won't find
# them, so we set ORBITAL_DATA_DIR to the source root.
test_env = environment()
test_env.set('ORBITAL_DATA_DIR', meson.project_source_root())

# Generate MOC files for each test header with Q_OBJECT
gamelibrary_moc = qt6_mod.compile_moc(
    headers: ['include/test_gamelibrary.h'],
    include_directories: test_include,
    dependencies: [qt6_dep],
)

envvarschema_moc = qt6_mod.compile_moc(
    headers: ['include/test_envvarschema.h'],
    include_directories: test_include,
    dependencies: [qt6_dep],
)

presetmerge_moc = qt6_mod.compile_moc(
    headers: ['include/test_presetmerge.h'],
    include_directories: test_include,
    dependencies: [qt6_dep],
)

# Test executables
test_gamelibrary = executable(
    'test_gamelibrary',
    'src/test_gamelibrary.cpp',
    gamelibrary_moc,
    include_directories: test_include,
    dependencies: [liborbital_dep, qt6_dep],
)

test_envvarschema = executable(
    'test_envvarschema',
    'src/test_envvarschema.cpp',
    envvarschema_moc,
    include_directories: test_include,
    dependencies: [liborbital_dep, qt6_dep],
)

test_presetmerge = executable(
    'test_presetmerge',
    'src/test_presetmerge.cpp',
    presetmerge_moc,
    include_directories: test_include,
    dependencies: [liborbital_dep, qt6_dep],
)

# Run tests
test('game_library', test_gamelibrary, env: test_env)
test('env_var_schema', test_envvarschema, env: test_env)
test('preset_merge', test_presetmerge, env: test_env)
