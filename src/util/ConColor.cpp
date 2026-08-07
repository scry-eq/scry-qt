#include "util/ConColor.h"
#include <algorithm>

Con conOf(int playerLevel, int spawnLevel) {
    if (playerLevel <= 0 || spawnLevel <= 0)
        return Con::White;

    const int diff = spawnLevel - playerLevel;
    if (diff >= 6) return Con::Red;
    if (diff >= 1) return Con::Yellow;
    if (diff == 0) return Con::White;

    // Lowest spawn level that cons light blue / green.
    const int cyanBase = std::min(
        playerLevel <= 60 ? (3 * playerLevel) / 4 : playerLevel - 15,
        playerLevel - 5);
    const int greenBase = std::min(
        playerLevel <= 60 ? (2 * playerLevel) / 3 : playerLevel - 20,
        cyanBase);

    if (spawnLevel >= playerLevel - 5) return Con::Blue;
    if (spawnLevel >= cyanBase)        return Con::Cyan;
    if (spawnLevel >= greenBase)       return Con::Green;
    return Con::Gray;
}

QColor conColor(Con c) {
    switch (c) {
    case Con::Gray:   return QColor(0x80, 0x80, 0x80);
    case Con::Green:  return QColor(0x00, 0xb0, 0x50);
    case Con::Cyan:   return QColor(0x00, 0xe0, 0xe0);
    case Con::Blue:   return QColor(0x40, 0x60, 0xff);
    case Con::Yellow: return QColor(0xff, 0xd0, 0x40);
    case Con::Red:    return QColor(0xff, 0x30, 0x30);
    case Con::White:  break;
    }
    return QColor(0xff, 0xff, 0xff);
}
