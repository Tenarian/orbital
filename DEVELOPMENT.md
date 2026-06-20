# Orbital Game Launcher — Development Guide

## Prerequisites

- **C++20** compiler (GCC 13+ or Clang 16+)
- **Meson** ≥ 1.0
- **Ninja** ≥ 1.10
- **Qt 6.6+** — modules: `Core`, `Network`, `DBus`, `Test`, `Quick`, `Qml`, `Widgets`

## Building

### Configure

```bash
# Default: GUI + CLI enabled
meson setup build

# GUI only
meson setup build -Dgui=true -Dcli=false

# CLI only
meson setup build -Dgui=false -Dcli=true

# No GUI, no CLI (liborbital only)
meson setup build -Dgui=false -Dcli=false
```

### Compile

```bash
meson compile -C build
```

### Install

```bash
meson install -C build
```

## Testing

```bash
# Run all tests
meson test -C build

# Run a specific test
meson test -C build orbital:GameLibrary
meson test -C build orbital:EnvVarSchema
meson test -C build orbital:PresetMerge

# Verbose output
meson test -C build -v
```

## Code structure

```
lib/        ← liborbital.so (shared library, all business logic)
cli/        ← orbital CLI binary
gui/        ← orbital-gui QML application
tests/      ← QTest-based unit tests
```
