# scry-qt

Qt6 GUI client — connects to a scry daemon (`scry-cpp` or
`scry-cpp-quarm`) over WebSocket. Daemon-agnostic, same as `scry-web`; the
user picks a daemon URL at runtime, never at build time.

## Stack

- Qt6 only, `find_package(Qt6 ... Core Network Xml WebSockets)` +
  `Qt6::Widgets` for the GUI itself.
- No pcap, no capabilities needed — this is a pure WebSocket client.
- `proto/` is **not** a submodule here; `seq-proto` regenerates `.pb.cc`
  per build.

## Structure

- `src/` — GUI client sources
- `proto/` — `seq-proto`, regenerated at build time (no submodule)
- `packaging/` — `build-appimage.sh` + AppImage packaging assets
- `cmake/` — CMake helper modules
- `tests/` — ctest suite (`concolor_test`, opt-in on `Qt6::Test`)

## Commands

- Local dev: `cmake -B build && cmake --build build`, then run
  `./build/scry-qt`. Daemon URL is chosen in the in-app Connect dialog
  (default `ws://127.0.0.1:9090`, persisted in `Settings`) — there is no
  CLI url flag.
- AppImage: `cmake --build build --target appimage` → produces
  `build/scry-qt-*.AppImage`. Driver is `packaging/build-appimage.sh`; it
  fetches linuxdeploy + linuxdeploy-plugin-qt on demand into `build/tools/`.

## Conventions

- Con colors live in `src/util/ConColor.h` and are **derived from the EQ
  client's runtime con table**, not a hand-tuned ladder. Three other ports
  of the same formula exist — `../scry-web/src/ui/concolor.ts` (canonical),
  `../iced-miseru/src/concolor.rs`, `../legacy/ShowEQ-Legends/src/player.cpp`
  — change them together. Daemons deliberately carry no con logic: level
  ships raw and each client colors it.
- Spawn list con-colors row text the way `legacy/ShowEQ-Legends` does
  (`src/spawnlistcommon.cpp`'s `m_textColor = pickConColor`) — corpses
  still dim to grey as a type override — plus a leading `ColCon` con-dot
  column carried over from `scry-web`. The map draws per-type glyphs:
  con-filled circle for NPCs, con-filled square with magenta outline for
  PCs, hollow yellow square for PC corpses, cyan plus for NPC corpses, tiny
  brown square for doors, yellow X for drops. Only NPCs go through the
  batched `drawPoints` path — the only populous type, so the rest are drawn
  per-item on purpose.

## Gotchas

- **AppImage CI container is `debian:bookworm-slim`** — do NOT use
  `ubuntu:22.04` (jammy lacks `qt6-websockets-dev`; only in noble/24.04+).
  Bookworm has it and keeps glibc at 2.36 for broad distro coverage. Set
  `APPIMAGE_EXTRACT_AND_RUN=1` in GHA containers (no FUSE there).
- **Windows CI** uses `jurplel/install-qt-action` for prebuilt Qt6 + a
  from-source protobuf build cached by tag. vcpkg was removed — Qt6 was 90%
  of the build time under it, and Microsoft retired the `x-gha` binary
  cache backend, so `lukka/run-vcpkg` rebuilt every run (~2.5h). Bumping
  the protobuf tag invalidates the cache; first build after that takes
  ~7-10 min, subsequent ~2-3 min.
- Linux CI jobs use system Qt6 + system protobuf (versions vary by
  distro) — cross-platform protobuf-version drift is cosmetic since
  `seq-proto` regenerates `.pb.cc` per build.
- `ctest` runs on the two Linux CI jobs only; `concolor_test` is opt-in on
  `find_package(Qt6 COMPONENTS Test)` so a qtbase split without `Qt6::Test`
  still builds the app.

## Documentation

- CI publishes a rolling `continuous` GitHub prerelease on every push to
  `main`, gated on the ubuntu/fedora/windows/appimage jobs:
  `https://github.com/scry-eq/scry-qt/releases/download/continuous/scry-qt-continuous-x86_64.AppImage`.
