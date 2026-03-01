# CITADEL — Claude Code Project Rules

## Project Overview
CITADEL is an ESP32 Arduino sketch — a retro Windows-style terminal with WiFi, BT, SYS,
AUTH, and ATTACK modules. Used in consensual, private, educational hacking competitions.

## File Structure
All files are .ino and compile as one translation unit in alphabetical order.
Do NOT add #include between files — all globals are shared automatically.

| File | Contents |
|------|----------|
| `CIT_V_2_0_0.ino` | Master — includes, types, globals, forward declarations |
| `01_utils.ino` | Logging, parsing, auth helpers, flash storage, clock |
| `02_hardware.ino` | OLED, health monitor, thermal, BLE keyboard |
| `03_attacks.ino` | Evil twin, deauth, beacon, captive portal |
| `04_commands.ino` | Full runCommand() — all command namespaces |
| `05_ui.ino` | Login page HTML |
| `06_server.ino` | Server routes, OTA setup, setup(), loop() |
| `07_github_ota.ino` | GitHub OTA — version check, fetch, self-flash |

## Code Rules
- Correctness first, then low RAM/flash, then maintainability
- NO dynamic allocation — static buffers only
- Change ONLY what is requested — do not reformat untouched sections
- Do not invent features not asked for
- ArduinoDroid compatible only
- Never use delay() in loops — use safeDelay()

## Version Naming
Format: CIT_V_X_Y_Z
- X: Major structural change
- Y: New feature added
- Z: Revision

Current version: CIT_V_2_1_0

## How to Release
1. Edit the relevant .ino file(s)
2. Update version string in CIT_V_2_0_0.ino and 06_server.ino
3. Commit: git commit -m "CIT_V_X_Y_Z — description"
4. Tag to trigger build: git tag vX.Y.Z && git push origin vX.Y.Z
5. GitHub Actions compiles and releases the .bin
6. ESP detects new version on next boot and flashes itself

## GitHub OTA
- Boot check runs in checkGithubOta() in 07_github_ota.ino
- GitHub PAT stored in Preferences key: gh_token
- Set via CITADEL terminal: OTA GITHUB TOKEN "your_pat"
- Repo: https://github.com/ggreg04/CITADEL

## Key Globals
- staConnected — bool, true if STA WiFi is up
- prefs — Preferences object for flash storage
- addLog(String) — adds to terminal log
- oledDraw(l1, l2, l3) — draws 3 lines to OLED
