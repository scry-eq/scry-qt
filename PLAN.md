# Plan: showeq-qt — Modern Qt6 Client for showeq-daemon

## Context
The legacy `showeq/` app is a Qt3/Qt4 autoconf C++ monolith: pcap capture, packet decode, and UI all in one process. The modern stack splits capture+decode into `showeq-daemon` (headless, WebSocket+protobuf). This plan creates `showeq-qt` — a native Qt6 desktop client that consumes the daemon over WebSocket, preserving the dockable-panel feel of the legacy app while targeting both Linux and Windows.

## User Decisions
- **Repo name**: `showeq-qt` → lives at `/home/rschultz/src/showeq/showeq-qt/`
- **Map data**: Proto `MapGeometry` only (no local .txt map file loading in v1)
- **Daemon connection**: Configurable `ws://HOST:PORT` (not hardcoded localhost)
- **v1 scope**: SpawnList + Map fully functional; all other panels as labeled placeholder docks

---

## Tech Stack

| Concern | Choice | Rationale |
|---|---|---|
| Build | CMake 3.21+ | Cross-platform; same as daemon; vcpkg manifest support |
| Qt | Qt 6.5 LTS | LTS through 2026; QDockWidget, QGraphicsView, QAbstractItemModel stable |
| Network | `QWebSocket` (QtWebSockets) | Binary protobuf frames; same pattern as daemon |
| Serialization | libprotobuf (proto3) | Shared `showeq-proto` git submodule |
| Settings | `QSettings` (INI) | Cross-platform; no custom XML needed |
| Dep management | **vcpkg** (manifest mode) | Best Win+Linux story for Qt6 + protobuf |
| CI | GitHub Actions (ubuntu-latest + windows-latest + fedora:latest container) | Three-platform build from day one |
| Packaging | CPack (NSIS on Win, DEB on Linux) | Ships with CMake; zero extra tooling |

---

## Repo Layout

```
showeq-qt/
├── CMakeLists.txt
├── vcpkg.json               # Qt6[core,widgets,websockets], protobuf
├── vcpkg-configuration.json
├── cmake/
│   └── FindProtobuf.cmake   # fallback if system protobuf
├── proto/                   # git submodule → showeq-proto (seq/v1/*.proto)
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── MainWindow.{h,cpp}        # QMainWindow: dock layout, menus, settings restore
│   │   ├── ConnectDialog.{h,cpp}     # host:port input, connect/disconnect button
│   │   └── Settings.{h,cpp}          # QSettings thin wrapper (keys as constexpr)
│   ├── daemon/
│   │   ├── DaemonConnection.{h,cpp}  # QWebSocket client, binary proto parse, reconnect timer
│   │   ├── SpawnModel.{h,cpp}        # QAbstractItemModel (spawn table; sort/filter proxy)
│   │   ├── ZoneState.{h,cpp}         # MapGeometry + spawn point cache
│   │   └── PlayerState.{h,cpp}       # PlayerStats, buffs, inventory (stub in v1)
│   └── widgets/
│       ├── SpawnListWidget.{h,cpp}   # QTreeView + proxy model + search bar [v1 FULL]
│       ├── MapWidget.{h,cpp}         # QGraphicsView: map lines, spawn dots, player dot [v1 FULL]
│       ├── CompassWidget.{h,cpp}     # QPainter stub dock
│       ├── PlayerStatsWidget.{h,cpp} # stub dock
│       ├── SpellListWidget.{h,cpp}   # stub dock
│       ├── MessageWindow.{h,cpp}     # stub dock
│       └── NetworkDiagWidget.{h,cpp} # stub dock
├── resources/
│   └── icons/               # QRC-embedded app icon (Win .ico + Linux .png)
└── .github/
    └── workflows/
        └── ci.yml           # matrix: ubuntu-latest, windows-latest, fedora:latest (container on ubuntu-latest runner); uploads artifacts
```

---

## DaemonConnection Design
```cpp
class DaemonConnection : public QObject {
  Q_OBJECT
signals:
  void connectionStateChanged(bool connected, QString sessionId);
  void snapshotReceived(seq::v1::Snapshot);
  void spawnAdded(seq::v1::SpawnAdded);
  void spawnUpdated(seq::v1::SpawnUpdated);
  void spawnRemoved(quint32 id);
  void zoneChanged(seq::v1::ZoneChanged);
  void playerStatsUpdated(seq::v1::PlayerStats);
  void mapGeometryReceived(seq::v1::MapGeometry);
  void chatMessage(seq::v1::ChatMessage);
public slots:
  void connectTo(QUrl url);   // ws://HOST:PORT
  void disconnect();
};
```
- Sends `ClientEnvelope{Subscribe{topics:[SPAWNS,ZONE,PLAYER,CHAT]}}` on connect
- Stores `session_id` + `last_seq` for resume on reconnect
- QTimer-based exponential backoff reconnect (1s → 2s → 4s → max 30s)

## SpawnModel Design
- `QAbstractItemModel` with columns: Name, Level, Class, Race, HP%, X, Y, Distance
- `QSortFilterProxyModel` sub-class for live filter bar (substring match on name/class/race)
- Updated via `spawnAdded` / `spawnUpdated` / `spawnRemoved` signals; batches repaints with `layoutChanged` debounce

## MapWidget Design
- `QGraphicsScene` + `QGraphicsView` with mouse pan (drag) and wheel zoom
- On `mapGeometryReceived`: render `MapLine[]` as `QGraphicsLineItem` grouped by layer
- On `mapGeometryReceived`: render `MapLocation[]` as small text labels
- Spawn dots: `QGraphicsEllipseItem` per spawn; color by con (grey/green/blue/white/yellow/red)
- Player position: distinct dot + heading tick; updated on each `spawnUpdated` for player id
- EQ coordinate system: X = east/west, Y = north/south; Y-axis inverted vs screen → apply negation in scene

## Settings Persisted (QSettings / INI)
- `daemon/url` — ws://host:port (default `ws://127.0.0.1:9090`)
- `mainwindow/geometry`, `mainwindow/state` — dock layout
- `spawnlist/columns` — visible columns + widths
- `spawnlist/filter` — last filter text
- `map/bgcolor`, `map/playercolor`, `map/spawncolors/*`

---

## Init Scaffolding (git + CMake)

```bash
git init showeq-qt
cd showeq-qt
git submodule add <showeq-proto-remote> proto
# vcpkg as submodule for reproducibility
git submodule add https://github.com/microsoft/vcpkg vcpkg
./vcpkg/bootstrap-vcpkg.sh
```

`vcpkg.json`:
```json
{
  "name": "showeq-qt",
  "version": "0.1.0",
  "dependencies": [
    { "name": "qtbase", "default-features": false, "features": ["widgets","websockets"] },
    "protobuf",
    "zlib"
  ]
}
```

---

## Phase Plan

| Phase | Deliverable |
|---|---|
| 1 | Repo scaffold: CMake, vcpkg, proto submodule, CI skeleton, `git init` |
| 2 | `DaemonConnection` — connect, decode, all signals, reconnect logic |
| 3 | `SpawnModel` + `SpawnListWidget` — sortable, filterable table |
| 4 | `MapWidget` — geometry lines, spawn dots, player dot, pan/zoom |
| 5 | `MainWindow` — dock layout, menus, `ConnectDialog`, settings persist |
| 6 | Stub docks (Compass, Stats, Spells, Messages, NetDiag) |
| 7 | CPack packaging + CI artifact upload (deb + Windows installer) |

---

## Verification
- **Ubuntu / Fedora**: `cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake && cmake --build build`, run `./build/showeq-qt`, connect to live daemon. Fedora CI job runs `fedora:latest` as a Docker container on an `ubuntu-latest` runner — no native Fedora runner exists on GitHub Actions.
- **Windows**: same CMake invocation with MSVC (or MinGW); CI matrix confirms clean build on `windows-latest`
- **Functional test**: run daemon with `--replay <fixture.vpk>`, connect Qt client, verify spawn table populates and map renders zone lines with spawn dots moving
- **Reconnect test**: kill daemon, watch client retry; restart daemon, verify client resumes with `session_id`
