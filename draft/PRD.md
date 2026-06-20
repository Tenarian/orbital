# Orbital Game Launcher — Product Requirement Document

## Name & Concept

- **Name:** Orbital Game Launcher
- **Binary/CLI:** `orbital`
- **Tagline:** *"For the games that launch from orbit."*
- **Concept:** Steam is the center (Earth), Orbital Game Launcher is a satellite orbiting it — and from that orbit, it launches games down
- **Reference:** Heroic Game Launcher (same generation, different personality)

---

## Tech Stack

| Layer | Tech |
|---|---|
| Frontend | QML |
| Backend | C++ |
| Build system | Meson + Qt6 |
| Storage | JSON files (QJsonDocument) |
| HTTP | QNetworkAccessManager |
| Packaging | AppImage / Flatpak |

---

## Architecture

```
┌─────────────┐     ┌──────────────┐     ┌─────────────────┐
│  QML GUI    │     │  CLI         │     │  Quickshell /   │
│  (official) │     │  `orbital`   │     │  3rd party UI   │
└──────┬──────┘     └──────┬───────┘     └────────┬────────┘
       │                   │                      │
       └───────────────────┼──────────────────────┘
                           │
                  ┌────────▼────────┐
                  │  liborbital.so  │  ← shared library, all business logic
                  │                 │
                  │  ├── GameLibrary
                  │  ├── ProtonManager
                  │  ├── PrefixManager
                  │  ├── ProtonDBClient
                  │  ├── EnvVarSchemaLoader
                  │  └── ProcessLauncher
                  └────────┬────────┘
                           │ D-Bus (IPC for 3rd party)
                    JSON / XDG paths
```

Both GUI and CLI link against `liborbital`. Third-party tools communicate via **D-Bus** (natively supported by Quickshell).

---

## Features

### Game Library
- Add/remove non-Steam games (executable path, cover art, metadata)
- Auto-fetch metadata by game name (IGDB / SteamGridDB)
- Grid/list view, search, filter, sort
- UX patterns borrowed from Heroic

### Library UX (based on Steam)

Layout borrowed from Steam, extended with things Steam doesn't have:

```
┌─────────────────────────────────────────────────────┐
│  [hero image]                          [Elden Ring] │
│                                           [▶ Play]  │
├──────────────┬──────────────────────────────────────┤
│  HLTB        │  Proton: GE-Proton9-7    [change]   │
│  Main: 45h   │  Prefix: per-game        [open]     │
│  Extras: 80h │  Last played: 2 days ago            │
│  100%: 130h  │                                     │
├──────────────┴──────────────────────────────────────┤
│  [Game Settings]  [Env Vars]  [Prefix]  [ProtonDB] │
└─────────────────────────────────────────────────────┘
```

- **Left sidebar** — game list, search, filter by category/tag
- **Hero banner** — fetched from SteamGridDB, logo overlay
- **HLTB block** — Main / Main+Extras / Completionist shown below hero
- **Proton + Prefix** — visible at a glance, changeable without going into settings
- **Tab bar** — Game Settings, Env Vars, Prefix, ProtonDB
- Grid view / List view toggle

### Game Setting Presets

**Community presets** — fetched from the sub-repo `orbital-game-launcher/presets`:

```
presets/
  ├── index.json        ← full preset list, used for search
  └── games/
      ├── elden-ring.json
      └── cyberpunk-2077.json
```

Preset file format:
```json
{
  "name": "Elden Ring",
  "steamAppId": 1245620,
  "contributors": ["user1", "user2"],
  "updatedAt": "2024-06-01",
  "presets": [
    {
      "id": "default",
      "label": "Default (AMD)",
      "description": "Works well on AMD GPU with GE-Proton",
      "protonVersion": "GE-Proton9-7",
      "envVars": {
        "PROTON_ENABLE_WAYLAND": "1",
        "MANGOHUD": "1",
        "RADV_PERFTEST": "aco"
      },
      "tags": ["amd", "wayland"]
    }
  ]
}
```

**Local presets** — user saves current config as a preset, can export/import JSON to share or submit to the sub-repo via PR.

**Cache** — preset index is cached locally, refreshed periodically or manually; no constant internet connection required. Stored under `~/.cache/orbital/presets/` (see XDG layout below).

**Multi-select + conflict resolution** — user can select multiple presets at once; the app merges them and shows a conflict resolver for any duplicate keys:

```
┌──────────────────────────────────────────────────────┐
│  Apply Presets — Elden Ring                          │
├──────────────────────────────────────────────────────┤
│  Merging 3 presets — 2 conflicts found               │
│                                                      │
│  PROTON_ENABLE_WAYLAND                               │
│  ○ "1"  ← Default (AMD)                             │
│  ● "0"  ← Nvidia + DLSS                             │
│                                                      │
│  RADV_PERFTEST                                       │
│  ● "aco"      ← Default (AMD)                       │
│  ○ "aco,nggc" ← MangoHud Full                       │
│                                                      │
│  Non-conflicting vars: 6  [show]                     │
├──────────────────────────────────────────────────────┤
│               [Cancel]  [Apply with selections]      │
└──────────────────────────────────────────────────────┘
```

Conflicts are automatically pre-selected to the value from the **last chosen preset** (last-write-wins); user overrides only if needed.

**Preset picker UI** (shown when user clicks "Browse Presets" on a game):

```
┌─────────────────────────────────────────────┐
│  Presets — Elden Ring           [🔄 Refresh]│
├─────────────────────────────────────────────┤
│  🔍 Filter presets...                       │
├─────────────────────────────────────────────┤
│  ☐  Default (AMD)          amd wayland      │
│     Works well on AMD + GE-Proton           │
│                                             │
│  ☐  Nvidia + DLSS          nvidia dlss      │
│     Enable DLSS via DXVK_NVAPI              │
│                                             │
│  ☐  MangoHud Full          mangohud perf    │
│     Full performance overlay                │
│                                             │
│  ── Local ──────────────────────────────── │
│  ☐  My Config (local)         [Export] [🗑] │
├─────────────────────────────────────────────┤
│  3 selected  [Apply Selected →]             │
└─────────────────────────────────────────────┘
```

**Fetch flow:**

```
startup
  └─ PresetManager::init()
       ├─ load local cache (index.json)
       ├─ check cache age > 24h OR manual refresh requested
       │    └─ fetch index from GitHub raw URL
       │         → write to ~/.cache/orbital/presets/index.json
       └─ cache ready (per-game preset files fetched lazily)
```

### SteamGridDB Integration
- Fetch cover art, hero images, logos, and icons for non-Steam games
- Show multiple artwork options → user picks their preferred one
- Automatic fallback if no artwork is found
- Requires a free API key stored in `config.json` as `"steamGridDbApiKey"`

**Artwork picker UI** — shown as a sheet/dialog after adding a game:

```
┌──────────────────────────────────────────────┐
│  Choose Artwork — Elden Ring                 │
├──────────────────────────────────────────────┤
│  Cover Art          Hero Image               │
│  ┌──┐ ┌──┐ ┌──┐    ┌────┐ ┌────┐            │
│  │  │ │  │ │  │    │    │ │    │            │
│  │✓ │ │  │ │  │    │ ✓  │ │    │            │
│  └──┘ └──┘ └──┘    └────┘ └────┘            │
│                                              │
│  Logo                Icon                   │
│  [Elden Ring SVG] [no logo found]  🔲 🔲    │
├──────────────────────────────────────────────┤
│              [Skip]  [Apply Selected]        │
└──────────────────────────────────────────────┘
```

Downloaded artwork cached to `~/.cache/orbital/artwork/{uuid}/{type}.{ext}`.

### HowLongToBeat Integration
- Fetch game completion times when adding to library:
  - **Main Story**
  - **Main + Extras**
  - **Completionist**
- Displayed in library view / game detail page
- Fetched by game name — no Steam App ID required
- Cached to `~/.cache/orbital/hltb/{uuid}.json`, TTL 30 days
- If no result found, shows "N/A" (no layout shift)

### Proton Manager
- Manage Proton versions (download/delete from GitHub releases)
- Scan `~/.steam/root/compatibilitytools.d/`
- Per-game Proton assignment
- **Auto-suggest Proton version** based on game name → query ProtonDB API

### ProtonDB Integration

**API endpoint:**
```
GET https://www.protondb.com/api/v1/reports/summaries/{steamAppId}.json
```

Tier ordering: `borked < bronze < silver < gold < platinum < native`

**Suggest logic:** resolves Steam App ID → fetches tier → maps to latest GE-Proton version. Shown as a suggestion chip, not auto-applied. If tier is `native`, no Proton is suggested.

**ProtonDB tab** — loads lazily when first selected, shows:
- Tier badge (colored by tier)
- Total report count
- Link out to ProtonDB page (`Qt.openUrlExternally`)

Results cached to `~/.cache/orbital/protondb/{uuid}.json`.

### Environment Variable Config (GUI)
- Schema-driven, data from JSON files
- Each env var renders as the appropriate widget type:
  - `toggle` → Switch
  - `slider` → Slider + value label
  - `multiselect` → CheckBox list
  - `gpu-select` → dropdown populated by Vulkan GPU detection
  - `text` → TextField fallback
- `dependsOn` — widget only shown when another var is enabled
- Categories: Proton, Vulkan, Mesa, AMD, Nvidia, MangoHud, Wine...

### Env Vars Tab
- Shows all current env vars (from GUI config + custom)
- **No read-only** — user can freely edit even generated vars
- 2 sections: Generated (from GUI config) and Custom (manually entered)
- Generated vars have a category badge → click to jump to that tab
- Bi-directional sync with GUI tabs

### CLI (`orbital`)

```bash
# library
orbital library list
orbital library search "ring"
orbital add "Elden Ring" /path/to/eldenring.exe
orbital launch "Elden Ring"

# proton
orbital proton suggest "Elden Ring"
orbital proton set "Elden Ring" GE-Proton9-7

# env vars
orbital env set "Elden Ring" MANGOHUD 1
orbital env list "Elden Ring"

# prefix
orbital prefix open "Elden Ring"
orbital prefix reset "Elden Ring"
orbital prefix run "Elden Ring" winecfg
orbital prefix shared list
orbital prefix shared create ea-app "EA App"
orbital prefix shared assign "It Takes Two" ea-app
orbital prefix shared run ea-app winecfg
```

### Prefix Manager

**Per-game prefix** — each game fully isolated:
```
~/.local/share/orbital/prefixes/by-game/{uuid}/
```

**Shared prefix** — group of games sharing one prefix (e.g. all EA games sharing a prefix with EA App pre-installed):
```
~/.local/share/orbital/prefixes/shared/{prefix-name}/
```

Each shared prefix has a `meta.json`:
```json
{
  "id": "ea-app",
  "name": "EA App",
  "description": "Shared prefix with EA App pre-installed",
  "protonVersion": "GE-Proton9-7",
  "games": ["550e8400-e29b-41d4-a716-446655440000", "6ba7b810-9dad-11d1-80b4-00c04fd430c8"]
}
```

Game config specifies prefix mode:
```json
{
  "prefixMode": "per-game"
}
```
or:
```json
{
  "prefixMode": "shared",
  "sharedPrefixId": "ea-app"
}
```

**Prefix tab UI** (inside GameDetailView):

```
┌─────────────────────────────────────────────┐
│  Prefix                                     │
├─────────────────────────────────────────────┤
│  Mode:  ● Per-game  ○ Shared               │
│                                             │
│  Path: ~/.local/share/orbital/prefixes/     │
│        by-game/550e8400-.../               │
│                                             │
│  [Open in File Manager]  [Reset Prefix]     │
│                                             │
│  Run tool in prefix:                        │
│  [winecfg]  [winetricks]  [explorer]       │
│  [Custom command...]                        │
└─────────────────────────────────────────────┘
```

Reset shows a confirmation dialog before deleting the prefix directory.

---

## Project Structure

```
orbital/
├── meson.build                  # root build file
├── meson.options                # build options (gui, cli toggles)
│
├── lib/                         # liborbital — core business logic
│   ├── meson.build
│   ├── include/orbital/         # public headers
│   │   ├── gamelibrary.h
│   │   ├── protonmanager.h
│   │   ├── prefixmanager.h
│   │   ├── protondbclient.h
│   │   ├── envvarschema.h
│   │   ├── processlauncher.h
│   │   ├── presetmanager.h
│   │   └── paths.h
│   └── src/
│       ├── gamelibrary.cpp
│       ├── protonmanager.cpp
│       ├── prefixmanager.cpp
│       ├── protondbclient.cpp
│       ├── envvarschema.cpp
│       ├── processlauncher.cpp
│       ├── presetmanager.cpp
│       └── paths.cpp
│
├── cli/                         # orbital binary
│   ├── meson.build
│   └── src/
│       ├── main.cpp
│       └── commands/
│           ├── library.cpp
│           ├── proton.cpp
│           ├── prefix.cpp
│           ├── env.cpp
│           └── preset.cpp
│
├── gui/                         # QML GUI
│   ├── meson.build
│   ├── src/
│   │   ├── main.cpp
│   │   └── bridge/              # C++ ↔ QML bridge
│   │       ├── librarymodel.h/cpp
│   │       ├── protonmodel.h/cpp
│   │       └── envvarmodel.h/cpp
│   └── qml/
│       ├── main.qml
│       ├── components/
│       │   ├── GameCard.qml
│       │   ├── HeroView.qml
│       │   ├── EnvVarWidget.qml
│       │   └── ConflictResolver.qml
│       └── views/
│           ├── LibraryView.qml
│           ├── GameDetailView.qml
│           ├── SettingsView.qml
│           └── ProtonManagerView.qml
│
├── assets/
│   ├── schema/                  # built-in env var schemas
│   │   ├── proton.json
│   │   ├── vulkan.json
│   │   ├── mesa.json
│   │   ├── radeon.json
│   │   ├── nvidia.json
│   │   ├── mangohud.json
│   │   └── wine.json
│   └── icons/
│
└── tests/
    ├── meson.build
    ├── test_gamelibrary.cpp
    ├── test_envvarschema.cpp
    └── test_presetmerge.cpp
```

Dependency graph:
```
liborbital.so  ←  Qt6::Core, Qt6::Network
orbital (CLI)  ←  liborbital
orbital-gui    ←  liborbital, Qt6::Quick, Qt6::Qml
```

GUI never links directly against Qt6::Network — all network calls go through liborbital.

---

## XDG Directory Layout

| Purpose | Default path | XDG override |
|---|---|---|
| Launcher config + user schemas | `~/.config/orbital/` | `$XDG_CONFIG_HOME/orbital/` |
| Game data + prefixes | `~/.local/share/orbital/` | `$XDG_DATA_HOME/orbital/` |
| Cached remote data | `~/.cache/orbital/` | `$XDG_CACHE_HOME/orbital/` |

### `~/.config/orbital/` — launcher config only

```
~/.config/orbital/
  ├── config.json           # global settings
  └── schema/               # user custom env var schemas (override/extend built-ins)
      └── custom.json
```

### `~/.local/share/orbital/` — game data + prefixes

```
~/.local/share/orbital/
  ├── games/
  │   ├── by-uuid/
  │   │   ├── 550e8400-e29b-41d4-a716-446655440000/
  │   │   │   └── metadata.json
  │   │   └── 6ba7b810-9dad-11d1-80b4-00c04fd430c8/
  │   │       └── metadata.json
  │   └── by-name/
  │       ├── Elden Ring -> ../by-uuid/550e8400-.../      # relative symlink to dir
  │       └── Cyberpunk 2077 -> ../by-uuid/6ba7b810-.../
  └── prefixes/
      ├── by-game/
      │   └── 550e8400-e29b-41d4-a716-446655440000/      # per-game Wine prefix
      └── shared/
          ├── ea-app/
          │   ├── drive_c/
          │   └── meta.json
          └── ubisoft/
              ├── drive_c/
              └── meta.json
```

The game UUID directory can hold additional per-game files in the future without restructuring:
```
550e8400-.../
  ├── metadata.json
  ├── presets/          ← local presets for this game (future)
  └── artwork/          ← game-specific artwork overrides (future)
```

### `~/.cache/orbital/` — all remote-fetched data

```
~/.cache/orbital/
  ├── artwork/              # SteamGridDB
  │   ├── 550e8400-.../
  │   │   ├── grid.png      # cover art (600x900)
  │   │   ├── hero.png
  │   │   ├── logo.png
  │   │   └── icon.png
  │   └── 6ba7b810-.../
  ├── hltb/                 # HowLongToBeat, TTL 30 days
  │   ├── 550e8400-....json
  │   └── 6ba7b810-....json
  ├── protondb/             # ProtonDB summaries
  │   ├── 550e8400-....json
  │   └── 6ba7b810-....json
  └── presets/              # community presets from sub-repo, TTL 24h
      ├── index.json
      └── games/
          ├── elden-ring.json
          └── cyberpunk-2077.json
```

Deleting `~/.cache/orbital/` entirely is safe — triggers re-fetch on next access.

### Path resolution in C++

```cpp
// lib/include/orbital/paths.h

class OrbitalPaths {
public:
    static QString config()  { return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation); }
    static QString data()    { return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation); }
    static QString cache()   { return QStandardPaths::writableLocation(QStandardPaths::CacheLocation); }

    // Derived paths — all computed, never hardcoded elsewhere
    static QString games()       { return data()  + "/games/by-uuid"; }
    static QString gamesByName() { return data()  + "/games/by-name"; }
    static QString prefixes()    { return data()  + "/prefixes"; }
    static QString artwork()     { return cache() + "/artwork"; }
    static QString hltbCache()   { return cache() + "/hltb"; }
    static QString protonDb()    { return cache() + "/protondb"; }
    static QString presets()     { return cache() + "/presets"; }
    static QString userSchemas() { return config() + "/schema"; }

    static QString builtinSchemas() {
        return QStandardPaths::locate(
            QStandardPaths::GenericDataLocation,
            "orbital/schema",
            QStandardPaths::LocateDirectory
        );
        // → /usr/share/orbital/schema in production
        // → ./assets/schema when running from build dir (via ORBITAL_DATA_DIR override)
    }
};
```

`QStandardPaths` handles `XDG_CONFIG_HOME`, `XDG_DATA_HOME`, and `XDG_CACHE_HOME` natively.

---

## Env Var Schema

### Folder structure

Built-in schemas ship with the app:
```
assets/schema/
  ├── proton.json
  ├── vulkan.json
  ├── mesa.json
  ├── radeon.json
  ├── nvidia.json
  ├── mangohud.json
  └── wine.json
```

Installed to `/usr/share/orbital/schema/`. User overrides: `~/.config/orbital/schema/` (merged on top at runtime).

### Schema format
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "x-orbital-category": {
    "id": "nvidia",
    "label": "Nvidia",
    "icon": "nvidia.svg",
    "description": "..."
  },
  "properties": {
    "PROTON_ENABLE_WAYLAND": {
      "type": "boolean",
      "default": false,
      "x-orbital": {
        "label": "Enable Wayland",
        "widget": "toggle",
        "description": "Run game natively on Wayland instead of XWayland"
      }
    },
    "DXVK_HUD": {
      "type": "string",
      "enum": ["fps", "frametimes", "devinfo", "..."],
      "x-orbital": {
        "widget": "multiselect",
        "separator": ",",
        "separator-policy": "lenient",
        "lenient-separators": [",", ":", "|", ", "]
      }
    },
    "PRIME_OFFLOAD": {
      "x-orbital": {
        "label": "PRIME Offload (prime-run)",
        "description": "Run game on discrete NVIDIA GPU instead of iGPU",
        "widget": "toggle",
        "type": "env-group",
        "vars": {
          "__NV_PRIME_RENDER_OFFLOAD": {
            "value": "1",
            "locked": true
          },
          "__NV_PRIME_RENDER_OFFLOAD_PROVIDER": {
            "widget": "gpu-select",
            "filter": "nvidia",
            "source": "runtime:gpu-list",
            "mapping": {
              "display": "{vendor} {name} ({vram}MB)",
              "value": "{pci_id}"
            }
          },
          "__GLX_VENDOR_LIBRARY_NAME": {
            "value": "nvidia",
            "locked": true
          }
        }
      }
    }
  }
}
```

### `env-group`
- Virtual key (e.g. `PRIME_OFFLOAD`) is never injected into the game; it only represents the group in the schema
- When toggled ON → expands into all vars listed under `vars`
- If user manually edits a var inside the group → group toggle shows `Modified` state
- If user manually deletes some vars → group toggle shows `Modified`, does not auto-toggle OFF

### `locked: true`
- Var is shown in the Env Vars tab with a 🔒 icon and cannot be edited
- Tooltip: "This value is managed by [group label] and cannot be changed"
- GUI-layer hint only — not enforced at runtime
- Can also be applied to standalone vars outside a group

### Separator policy
- **`lenient`** — accepts multiple separator formats when parsing (e.g. DXVK_HUD)
- **`strict`** — only accepts the canonical separator (e.g. RADV_PERFTEST)
- Determined per env var according to its own docs
- **No normalization** — raw user input is preserved; only validate and report errors

### Runtime Sources

Some widgets need dynamic option lists that cannot be hardcoded in the schema — e.g. the list of Vulkan devices, installed Wine versions, audio sinks. These are provided by the **runtime source system**, invoked via the `source` field on `x-orbital`:

```json
"MESA_VK_DEVICE_SELECT": {
  "type": "string",
  "x-orbital": {
    "label": "Vulkan Device",
    "widget": "gpu-select",
    "source": "runtime:gpu-list",
    "filter": "any",
    "mapping": {
      "display": "{vendor} {name} ({vram}MB)",
      "value": "{pci_id}"
    }
  }
}
```

- `source: "runtime:gpu-list"` → looks up `gpu-list` in the runtime source registry (populated by source plugins — see Plugin System below)
- `filter` → optional filter function applied to the source's items (e.g. `"nvidia"`, `"amd"`, `"any"`)
- `mapping.display` → format string for the human-readable label shown in the dropdown
- `mapping.value` → format string for the actual env var value written to metadata

When `source` is absent, `gpu-select` falls back to a built-in Vulkan device enumeration (legacy behavior, kept for backward compatibility).

### Format Strings & Placeholder Table

`mapping.display` and `mapping.value` use format strings with named placeholders enclosed in `{}`:

```
"{vendor} {name} ({vram}MB)"  →  "AMD Radeon RX 7900 XTX (24576MB)"
"{pci_id}"                    →  "1002:744c"
```

Placeholders are resolved against a registered **placeholder table** at runtime. The table is populated by placeholder plugins (see Plugin System below). Common placeholders include `vendor`, `name`, `vram`, `pci_id`, `driver`, `device_path` — but the set is extensible.

If a placeholder is not registered, the format string fails loudly at render time (the dropdown item shows `"<unresolved:placeholder>"` rather than silently producing a broken value).

---

## Plugin System

Orbital has two independent plugin systems. They are intentionally separate because they solve different problems. Both use a C ABI (`extern "C"`) to ensure stability across compiler versions and build configurations.

### Plugin Discovery

```
$ORBITAL_PLUGIN_PATH          # env override (highest priority)
~/.config/orbital/plugins/    # user plugins
/usr/lib/orbital/plugins/     # system plugins
```

All three paths are scanned at startup; `.so` files matching `orbital_plugin_*.so` or `orbital_placeholder_*.so` / `orbital_source_*.so` are loaded via `dlopen`. Each plugin's registration function is called exactly once.

### ABI Stability

- All plugin APIs use C ABI (`extern "C"`)
- Memory ownership is explicit: strings returned from plugin callbacks must be freed using the corresponding `free_*` function provided by the registry, not by the caller directly
- Structs passed across the boundary are versioned with a `version` field checked on load
- A plugin built against an older header may be rejected if its version is below the registry's minimum supported version

### 1. Placeholder Plugins

Extend the **format string placeholder table** used by `x-orbital` `mapping.display` / `mapping.value` resolution.

```cpp
// Plugin entry point — called once at load time
extern "C" int orbital_placeholders_register(OrbitalPlaceholderRegistry* reg);

// Example: a GPU info plugin providing vendor/vram/pci_id placeholders
extern "C" int orbital_placeholders_register(OrbitalPlaceholderRegistry* reg) {
    reg->add("vendor", gpu_resolve_vendor);
    reg->add("vram",   gpu_resolve_vram);
    reg->add("pci_id", gpu_resolve_pci_id);
    return ORBITAL_OK;
}
```

**Conflict resolution:** if two plugins register the same placeholder name, that bare placeholder is rejected entirely. To use it, the schema must reference it with an explicit `plugin-id:placeholder` prefix:

```
"vendor"                   → rejected (ambiguous)
"plugin-gpu-amd:vendor"    → resolved to plugin-gpu-amd's implementation
"plugin-gpu-nvidia:vendor" → resolved to plugin-gpu-nvidia's implementation
```

This means conflict resolution happens at schema authoring time, not at runtime. A schema that uses a bare placeholder name that happens to be ambiguous will fail loudly, forcing the author to be explicit about which plugin they intend.

### 2. Source Plugins

Extend the **runtime source registry** — i.e. provide the dynamic data that `source: "runtime:gpu-list"` refers to.

```cpp
extern "C" int orbital_sources_register(OrbitalSourceRegistry* reg);

extern "C" int orbital_sources_register(OrbitalSourceRegistry* reg) {
    reg->add_source("gpu-list", []() -> OrbitalDataList {
        return enumerate_vulkan_devices();
    });
    reg->add_source("wine-versions", []() -> OrbitalDataList {
        return enumerate_installed_wine_versions();
    });
    return ORBITAL_OK;
}
```

Each source returns an `OrbitalDataList` — a list of items where each item is a map of placeholder name → value. When the GUI renders a `gpu-select` widget bound to `source: "runtime:gpu-list"`, it calls the registered source, then applies `mapping.display` / `mapping.value` to each item to produce the dropdown.

Source plugins provide runtime data only. Schema definitions themselves live in `assets/schema/` and go through community review via PR (see Open Design Decisions) — they are not loaded from arbitrary `.so` files at runtime.

### Built-in Plugins

A small set of plugins ships with the launcher and is loaded automatically (no discovery scan needed):

| Plugin | Type | Provides |
|---|---|---|
| `orbital_placeholder_gpu` | placeholder | `vendor`, `name`, `vram`, `pci_id`, `driver`, `device_path` |
| `orbital_source_gpu` | source | `runtime:gpu-list` (enumerates Vulkan devices via `vulkaninfo` or direct ICD parse) |

These cover the common case (GPU selection) out of the box. Third-party plugins can extend or override them by registering the same names with higher-priority discovery paths (e.g. a user plugin in `~/.config/orbital/plugins/` shadows the built-in).

---

## Validation & Warning

### Env var states
| State | Meaning |
|---|---|
| `Clean` | GUI and Env Vars tab are in sync |
| `Modified` | User edited manually, value still parses |
| `Invalid` | User edited manually, value fails to parse |

### Invalid warning dialog
- Shown when launching a game with invalid env vars
- Lists each invalid var with its error and expected values
- Checkbox "Don't ask again" (per-game)
- Buttons: **[Cancel]** / **[Launch Anyway]**

### Suppress logic (priority order)
```
Global suppress ON   → never warn (overrides everything)
Per-game suppress ON → never warn
Default              → show dialog
```

- Per-game suppress does NOT auto-reset when a new invalid var is added
- Global suppress stored in `~/.config/orbital/config.json`
- Per-game suppress stored in game's `metadata.json`

---

## JSON Storage

### ID generation

UUID4 via `QUuid::createUuid().toString(QUuid::WithoutBraces)`.

### games/by-uuid/{uuid}/metadata.json

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "Elden Ring",
  "executable": "/games/eldenring/eldenring.exe",
  "protonVersion": "GE-Proton9-7",
  "launchArgs": "",
  "lastPlayed": 1718400000,
  "suppressEnvWarning": false,
  "prefixMode": "per-game",
  "sharedPrefixId": null,
  "envVars": {
    "PROTON_ENABLE_WAYLAND": "1",
    "MANGOHUD": "1",
    "MY_CUSTOM_VAR": "value"
  }
}
```

Artwork is stored in `~/.cache/orbital/artwork/{uuid}/` — not referenced by filename in metadata.

### config.json

```json
{
  "suppressEnvWarning": false,
  "sidebarCollapsed": false,
  "steamGridDbApiKey": "",
  "protonCheckUpdatesOnStartup": true,
  "schemaVersion": "1.0.0"
}
```

Flat key-value object — easy to extend with new settings.

`schemaVersion` pins the env var schema set the launcher expects. Schemas ship in `assets/schema/` within the project repo and are versioned via the launcher's own version bump (community contributions via PR — see Open Design Decisions). When the launcher starts, it checks that the installed schema set's reported version is compatible with `schemaVersion`; mismatches are logged as warnings, not fatal.

### Symlinks

`by-name/` symlinks point to the **directory** (not the file):
```
by-name/Elden Ring -> ../by-uuid/550e8400-.../
```

Resolve via: `by-name/{name}/metadata.json`

Symlinks are relative — portable when moving/backing up the entire data directory.

Duplicate names → append short UUID segment: `"Elden Ring (550e8400)"`

Special characters in names: `/` → `∕` (Unicode division slash), null bytes stripped.

### Atomic write pattern

```cpp
void GameLibrary::save(const QString &id) {
    QString dir  = OrbitalPaths::games() + "/" + id;
    QString path = dir + "/metadata.json";
    QString tmp  = path + ".tmp";

    QDir().mkpath(dir);
    QFile f(tmp);
    f.open(QIODevice::WriteOnly);
    f.write(QJsonDocument(toJson(findById(id))).toJson());
    f.close();
    QFile::rename(tmp, path);  // atomic on Linux (same filesystem)
}
```

### Startup load pattern

```cpp
void GameLibrary::load() {
    QDir dir(OrbitalPaths::games());
    for (const QString &uuid : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFile f(dir.filePath(uuid + "/metadata.json"));
        if (!f.open(QIODevice::ReadOnly)) continue;
        m_games.append(fromJson(QJsonDocument::fromJson(f.readAll()).object()));
    }
}
```

No index file needed — the directory listing is the index.

---

## C++ Key Types

```cpp
enum class EnvVarSource { Generated, Custom };
enum class EnvVarState  { Clean, Modified, Invalid };

struct EnvVarEntry {
    QString key;
    QString value;       // raw user input, never normalized
    EnvVarSource source;
    EnvVarState state;
    QString categoryId;  // if source == Generated
    QString groupId;     // if part of an env-group, the virtual key of that group
    bool locked = false; // true → shown with 🔒 in Env Vars tab, not editable
    QString errorMessage;
};

struct ParseResult {
    bool valid;
    QVariant parsedValue;  // used internally to sync GUI, never written back to raw value
    QString errorMessage;
};

struct LaunchConfig {
    QString executable;
    QStringList args;
    QString workingDir;
    QString protonPath;
    QString prefixPath;
    QMap<QString, QString> envVars;
    bool skipEnvValidation = false;
};

struct HLTBData {
    int mainSeconds;
    int mainPlusSeconds;
    int completionSeconds;
};
```

---

## ProcessLauncher

**Env build order** (later entries win):
```
1. System env (filtered — strip LD_PRELOAD, LD_LIBRARY_PATH, WINEPREFIX, WINEFSYNC, WINEESYNC)
2. STEAM_COMPAT_DATA_PATH, WINEPREFIX  ← from prefixPath
3. Game envVars from metadata (Generated + Custom, merged)
4. env-group expansion (virtual keys → real vars)
```

Filter deny-list lives in `assets/schema/launch-filter.json` (user-editable without recompiling).

**Launch:**
```cpp
proc->setProgram(config.protonPath);
proc->setArguments({"run", config.executable} + config.args);
proc->setProcessEnvironment(builtEnv);
proc->start();
```

---

## GameLibrary C++ Model

```cpp
class GameLibrary : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole, ExecutableRole, ProtonVersionRole,
        LastPlayedRole, PrefixModeRole
    };

    int      rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString          addGame(const GameEntry &entry);   // returns new UUID
    void             removeGame(const QString &id);
    void             updateGame(const GameEntry &entry);
    GameEntry        findById(const QString &id) const;
    GameEntry        findByName(const QString &name) const;
    QList<GameEntry> search(const QString &query) const;

    void load();
    void save(const QString &id);

signals:
    void gameAdded(const QString &id);
    void gameRemoved(const QString &id);
    void gameUpdated(const QString &id);
};
```

---

## QML Navigation Structure

```
main.qml
├── SidebarPanel (always visible, collapsible)
│   ├── search bar
│   ├── ListView → GameCard.qml
│   └── footer: [Add Game] [Settings]
│
└── StackView (right pane)
    ├── LibraryView.qml       ← landing / grid view
    ├── GameDetailView.qml    ← selected game
    │   ├── HeroView.qml
    │   ├── InfoBar.qml       ← HLTB + Proton + Prefix at a glance
    │   └── TabBar
    │       ├── GameSettingsTab.qml
    │       ├── EnvVarsTab.qml
    │       ├── PrefixTab.qml
    │       └── ProtonDBTab.qml
    ├── ProtonManagerView.qml
    └── SettingsView.qml
```

Navigation via named routes on StackView. Sidebar state (collapsed/expanded) persisted in `config.json`.

---

## D-Bus Interface

Service: `io.orbital.Launcher` / Object: `/io/orbital/Launcher`

```xml
<interface name="io.orbital.Launcher">
  <method name="ListGames">
    <arg direction="out" type="aa{sv}" name="games"/>
  </method>
  <method name="LaunchGame">
    <arg direction="in"  type="s" name="gameId"/>
    <arg direction="out" type="b" name="success"/>
  </method>
  <signal name="GameStarted">
    <arg type="s" name="gameId"/>
    <arg type="x" name="pid"/>
  </signal>
  <signal name="GameExited">
    <arg type="s" name="gameId"/>
    <arg type="i" name="exitCode"/>
  </signal>
  <signal name="LibraryChanged"/>
</interface>
```

Registered by `orbital-gui` on startup. v1 scope: read + launch only. Write methods (AddGame, SetProton, etc.) deferred to v2.

---

## meson.build

**Root:**
```meson
project('orbital', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++20', 'warning_level=2']
)

qt6 = import('qt6')
qt6_dep = dependency('qt6', modules: ['Core', 'Network'])

subdir('lib')
if get_option('cli')
  subdir('cli')
endif
if get_option('gui')
  subdir('gui')
endif
subdir('tests')

install_subdir('assets/schema',
  install_dir: get_option('datadir') / 'orbital' / 'schema'
)
```

**`meson.options`:**
```meson
option('gui', type: 'boolean', value: true,  description: 'Build QML GUI')
option('cli', type: 'boolean', value: true,  description: 'Build orbital CLI')
```

**`lib/meson.build`:**
```meson
liborbital = shared_library('orbital',
  files('src/gamelibrary.cpp', 'src/protonmanager.cpp', 'src/prefixmanager.cpp',
        'src/protondbclient.cpp', 'src/envvarschema.cpp', 'src/processlauncher.cpp',
        'src/presetmanager.cpp', 'src/paths.cpp'),
  include_directories: include_directories('include'),
  dependencies: [qt6_dep],
  install: true,
)

liborbital_dep = declare_dependency(
  link_with: liborbital,
  include_directories: include_directories('include'),
)
```

---

## Open Design Decisions

- **IGDB vs SteamGridDB for metadata** — SteamGridDB for artwork (free API key, simpler). IGDB opt-in in settings for richer metadata (description, genres, release year). Requires OAuth client credentials.
- **`orbital-daemon` vs GUI-only D-Bus** — GUI-only for v1. Document that D-Bus is unavailable when GUI is closed. Daemon deferred to v2.
- **GE-Proton update check** — via GitHub releases API on startup (toggleable in `config.json`).
- **Schema distribution** — Schemas ship in `assets/schema/` within the launcher repo (not a separate mono repo). Community contributions follow the same PR-based workflow as the `orbital-game-launcher/presets` sub-repo: contributors fork the launcher repo, edit schema files under `assets/schema/`, and open a PR for review. Once merged, the new schema lands in the next launcher release. This keeps schema and loader in lockstep (no version-skew risk) while still allowing community-driven schema growth. A separate `orbital-schemas` repo is deferred to v2 if/when contribution volume warrants it.
- **Plugin ABI versioning** — v1 ships a single frozen ABI. If breaking changes are needed later, bump the registration struct's `version` field and reject older plugins at load with a clear error message pointing the user to the plugin's upstream for a rebuild.

## Suggested Implementation Order

1. `OrbitalPaths` — foundation everything else calls
2. `GameLibrary` (load/save/list) + tests
3. `EnvVarSchemaLoader` (parse schemas → `EnvVarEntry` list)
4. `ProcessLauncher` (build env, launch via QProcess)
5. CLI (`orbital library list`, `orbital launch`) — validates lib API without QML
6. QML bridge models (`LibraryModel`, `EnvVarModel`)
7. QML views: `LibraryView` → `GameDetailView` → tabs
