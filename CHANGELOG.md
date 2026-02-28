# CITADEL CHANGELOG

## CIT_V_2_0_0 — 2025-02-28
**Checksum:** CLDTX6
**Type:** Major structural reorganization (no functional changes)

### Changes
Monolithic single-file sketch split into 7 files for maintainability:

| File | Lines | Contents |
|------|-------|----------|
| `CIT_V_2_0_0.ino` | 182 | Master — includes, types, globals, forward declarations |
| `01_utils.ino` | 238 | Logging, parsing, auth helpers, flash storage, clock, sensors |
| `02_hardware.ino` | 262 | Health monitor, thermal, OLED, BLE keyboard functions |
| `03_attacks.ino` | 118 | Evil twin, deauth, beacon, captive portal HTML builders |
| `04_commands.ino` | 1084 | Full `runCommand()` — all command namespaces |
| `05_ui.ino` | 427 | Login page HTML + terminal HTML/CSS/JS |
| `06_server.ino` | 421 | Server routes, OTA setup, `setup()`, `loop()`, build record |

**Total: 2,732 lines** (vs 2,741 original — difference is removed stale part markers)

### How It Works
ArduinoDroid compiles all `.ino` files in a sketch folder alphabetically and concatenates them into one translation unit. The numbered prefixes ensure correct compile order. No `#include` statements needed between files — all globals and functions are visible across files.

### Affected Areas
- All code — reorganized but not modified
- Version strings updated: `v2.1.0` → `v2.0.0`, `CLDTX5` → `CLDTX6`
- Stale "PASTE PART X" comments removed

### How to Flash
1. Create a new sketch folder named `CIT_V_2_0_0` in ArduinoDroid
2. Place all 7 `.ino` files inside it
3. Open `CIT_V_2_0_0.ino` (the master file) in ArduinoDroid
4. Compile and flash as normal

### Constraints Noted
- `runCommand()` uses `goto done` pattern — cannot be split across files, stays monolithic in `04_commands.ino`
- Future updates: replace just the affected file(s), not the whole sketch

### Problems Encountered
- None yet — needs compile test on device

### Lessons Learned
- Numbered prefixes are essential for ArduinoDroid compile order
- The `goto` pattern in `runCommand()` locks it into a single file
