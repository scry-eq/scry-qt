#pragma once
#include <QColor>

// EQ con-color bands, derived from the client's own runtime con table.
// Mirrors showeq-web/src/ui/concolor.ts and iced-miseru/src/concolor.rs —
// same palette, same bands. Keep all three in sync.
//
// Above the player the bands are flat: +1..+5 yellow, +6 and up red. Below
// the player the light-blue and grey bases scale as 3/4 and 2/3 of the
// player level until 60, then flatten to -15 and -20, each clamped against
// the band above it — so at low levels green and light blue vanish entirely
// (at level 10, 5-9 is dark blue and 1-4 grey).

enum class Con { Gray, Green, Cyan, Blue, White, Yellow, Red };

// Returns Con::White when either level is unknown (0).
Con conOf(int playerLevel, int spawnLevel);

QColor conColor(Con c);

inline QColor conColorOf(int playerLevel, int spawnLevel) {
    return conColor(conOf(playerLevel, spawnLevel));
}
