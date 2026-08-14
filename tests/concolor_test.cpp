// Pins the con bands against the client's runtime con table. The same
// cases are asserted in scry-web/src/ui/concolor.test.ts and
// iced-miseru/src/concolor.rs — keep the three in sync.
#include "util/ConColor.h"
#include <QTest>

class ConColorTest : public QObject {
    Q_OBJECT
private slots:
    void unknownLevelsAreWhite() {
        QCOMPARE(conOf(0, 10), Con::White);
        QCOMPARE(conOf(10, 0), Con::White);
    }

    void bandsAboveThePlayerAreFlat() {
        QCOMPARE(conOf(20, 20), Con::White);
        QCOMPARE(conOf(20, 21), Con::Yellow); // +1
        QCOMPARE(conOf(20, 25), Con::Yellow); // +5
        QCOMPARE(conOf(20, 26), Con::Red);    // +6
        QCOMPARE(conOf(20, 30), Con::Red);
    }

    // At low levels there is no room below for green or light blue, so
    // both bands clamp away entirely.
    void lowLevelsCompressToGreyAndBlue() {
        QCOMPARE(conOf(7, 1), Con::Gray);
        QCOMPARE(conOf(7, 2), Con::Blue);
        QCOMPARE(conOf(7, 6), Con::Blue);
        QCOMPARE(conOf(10, 4), Con::Gray);
        QCOMPARE(conOf(10, 5), Con::Blue);
        QCOMPARE(conOf(10, 9), Con::Blue);
        QCOMPARE(conOf(15, 9), Con::Gray);
        QCOMPARE(conOf(15, 10), Con::Blue);
    }

    // Levels where the legacy hand-tuned ladder was off by one at the
    // grey/green edge.
    void midLevelsMatchTheClientTable() {
        QCOMPARE(conOf(18, 11), Con::Gray);
        QCOMPARE(conOf(18, 12), Con::Green);
        QCOMPARE(conOf(21, 13), Con::Gray);
        QCOMPARE(conOf(21, 14), Con::Green);
        QCOMPARE(conOf(21, 15), Con::Cyan);
        QCOMPARE(conOf(21, 16), Con::Blue);
        QCOMPARE(conOf(28, 17), Con::Gray);
        QCOMPARE(conOf(28, 18), Con::Green);
        QCOMPARE(conOf(28, 20), Con::Green);
        QCOMPARE(conOf(28, 21), Con::Cyan);
        QCOMPARE(conOf(28, 22), Con::Cyan);
        QCOMPARE(conOf(28, 23), Con::Blue);
    }

    void level60ShowsTheUncompressedBands() {
        QCOMPARE(conOf(60, 39), Con::Gray);  // -21
        QCOMPARE(conOf(60, 40), Con::Green); // -20
        QCOMPARE(conOf(60, 44), Con::Green); // -16
        QCOMPARE(conOf(60, 45), Con::Cyan);  // -15
        QCOMPARE(conOf(60, 54), Con::Cyan);  // -6
        QCOMPARE(conOf(60, 55), Con::Blue);  // -5
        QCOMPARE(conOf(60, 59), Con::Blue);  // -1
    }

    // Past 60 the scaling bases flatten to fixed -15 / -20 offsets.
    void above60UsesTheFlatBases() {
        QCOMPARE(conOf(100, 79), Con::Gray);
        QCOMPARE(conOf(100, 80), Con::Green);
        QCOMPARE(conOf(100, 84), Con::Green);
        QCOMPARE(conOf(100, 85), Con::Cyan);
        QCOMPARE(conOf(100, 94), Con::Cyan);
        QCOMPARE(conOf(100, 95), Con::Blue);
    }
};

QTEST_APPLESS_MAIN(ConColorTest)
#include "concolor_test.moc"
