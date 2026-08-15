# scry-qt

Qt6 GUI client for Scry. Connects to a running daemon (`scry-cpp` for Live
EQ, `scry-cpp-quarm` for Project Quarm — daemon-agnostic, same as
`scry-web`) over WebSocket and speaks the same `seq.v1` protobuf contract:
spawn map, spawn list, player stats, spells, compass, chat/combat
messages.

## Build & run

Requires Qt 6.2+ (Core, Gui, Widgets, WebSockets) and a protobuf
toolchain.

```sh
git submodule update --init --recursive   # pulls scry-proto into proto/
cmake -B build
cmake --build build
./build/scry-qt
```

There's no CLI URL flag — pick the daemon URL in the in-app Connect dialog
(default `ws://127.0.0.1:9090`, persisted in `Settings`).

## AppImage

```sh
cmake --build build --target appimage
```

produces `build/scry-qt-*.AppImage` (fetches linuxdeploy on demand). CI
publishes a rolling `continuous` prerelease on every push to `main`:
`https://github.com/scry-eq/scry-qt/releases/download/continuous/scry-qt-continuous-x86_64.AppImage`.

## Layout

```
src/
  main.cpp              # entry point
  app/
    MainWindow.*         # top-level window, menu, panel layout
    ConnectDialog.*       # daemon URL picker
    Settings.*             # persisted UI prefs (QSettings)
  daemon/
    DaemonConnection.*    # WebSocket + seq.v1 protobuf client
    SpawnModel.*            # spawn table model
    PlayerState.*             # player stats/vitals
    ZoneState.*                # zone name + geometry
    races.h                     # race id -> name table
  widgets/
    MapWidget.*            # canvas-rendered spawn map (+ SmoothedPos movement lerp)
    SpawnListWidget.*        # spawn list panel
    PlayerStatsWidget.*        # HP/mana/stamina/exp bars
    CompassWidget.*              # heading compass
    SpellListWidget.*              # spell/buff list
    MessageWindow.*                  # chat/combat message log
    NetworkDiagWidget.*                # connection diagnostics
  util/
    ConColor.*             # con-color math (mirrors scry-web's concolor.ts)
proto/                    # git submodule -> scry-proto
packaging/                # AppImage build driver + assets
```

## License

MIT. See [LICENSE](LICENSE).
