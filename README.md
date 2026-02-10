LiveImproved is a system-level extension for Ableton Live focused on **real keyboard usability** and efficient plugin selection, including a configurable double-right-click plugin menu.

It fixes long-standing focus and window management issues, enables deterministic keyboard shortcuts and macros, and provides a fast, format-aware plugin launcher — all without relying on UI scripting or timing hacks.

LiveImproved is designed to eliminate the need for wrapper racks, duplicated plugins, or naming conventions to enable fast plugin access.

### Why?

Ableton Live is extremely powerful, but its keyboard usability breaks down once plugins and complex workflows are involved.
These issues have been among the most-requested features on Ableton’s [feature request forum](https://ableton.centercode.com) for over a decade.

LiveImproved fixes this by restoring reliable focus, window management, and deterministic keyboard control.

LiveImproved implements reliable focus, window management, and deterministic keyboard control at the system level.

### What LiveImproved enables

- True keyboard-driven workflows
  - Reliable focus and z-ordering for plugin windows
  - Keyboard cycling and closing (`` cmd+` ``, ``cmd+w``) without mouse interaction
- Deterministic actions
  - Custom shortcuts and macros that don’t depend on UI state
  - Automatically disabled while typing in text fields
- Designed with latency as the primary constraint
- Fast, intentional plugin access
  - Fuzzy-searchable plugin launcher
  - Customizable format priority (e.g. VST3 → AU → VST)
  - Direct plugin loading via IPC → custom MIDI Remote Script, not browser navigation
- Optional window orchestration
  - Open / close all plugins on a track
  - Tile plugin windows when needed
- Context-aware double-right-click plugin menu

### Example configuration

LiveImproved is configured via YAML.

Configuration is split into a keyboard/action config and a double-right-click menu config.

```yaml
remap:
  cmd+w: closeFocusedPlugin
  cmd+shift+w: closeAllPlugins
  d: delete
  cmd+i: cmd+a, plugin.Serum
  cmd+b: cmd+d, cmd+d, cmd+d, cmd+d
```

See the [example action config](https://github.com/ChasonDeshotel/LiveImproved-RemoteScript/blob/main/config.txt) and [example context menu config](https://github.com/ChasonDeshotel/LiveImproved-RemoteScript/blob/main/config-menu.txt) for more details.

### Migration from Live Enhancement Suite (LES)

LiveImproved includes tooling to import existing LES plugin menu configurations, allowing users to transition without rebuilding their workflows.

### Project status
- Actively used by the author
- Proven stable on macOS (Intel & Apple Silicon)
- Windows port in progress

### Platform support

- [x] macOS (Intel)
- [x] macOS (Apple Silicon)
- [ ] Windows
  - [x] IPC
  - [x] Keyboard hooking
  - [x] Accessibility / UI integration
  - [ ] Plugin window ordering & z-order management

### Building

```sh
# macOS:
brew install harfbuzz
brew install yaml-cpp
git submodule update --init --recursive
make configure
make configure-xcode
make build-xcode
# copy the contents of ./remote_scripts to <Ableton MIDI Remote Scripts>/LiveImproved
# or,
# cd <Ableton MIDI Remote Scripts> && git clone git@github.com:ChasonDeshotel/LiveImproved-RemoteScript.git LiveImproved
```

### Contributing

Contributions are welcome.

Remaining work is concentrated in the Live window management layer on Windows (focus, z-ordering, and plugin window enumeration).
