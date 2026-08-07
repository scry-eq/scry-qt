#pragma once
#include "seq/v1/events.pb.h"
#include <QElapsedTimer>
#include <QGraphicsView>
#include <QHash>
#include <QList>
#include <QPointF>
#include <array>

class QGraphicsScene;
class QGraphicsEllipseItem;
class QGraphicsItemGroup;
class QGraphicsLineItem;
class QTimer;

class MapWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapWidget(QWidget* parent = nullptr);

public:
    void centerOnSpawn(quint32 id);
    void setShowGrid(bool on) { m_showGrid = on; viewport()->update(); }
    bool showGrid() const { return m_showGrid; }
    void setShowFps(bool on) { m_showFps = on; viewport()->update(); }
    bool showFps() const { return m_showFps; }

public slots:
    void applyGeometry(const seq::v1::MapGeometry&);
    void applySnapshot(const seq::v1::Snapshot&);
    void applySpawnAdded(const seq::v1::SpawnAdded&);
    void applySpawnUpdated(const seq::v1::SpawnUpdated&);
    void applySpawnRemoved(quint32 id);
    void setPlayerId(quint32 id);

protected:
    void paintEvent(QPaintEvent*) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    static QPointF eqToScene(float x, float y);
    static QColor  colorForSpawnType(seq::v1::SpawnType);
    void drawPlayerMarker(QPainter* painter);
    void onRenderTick();

    QGraphicsScene* m_scene;
    quint32 m_playerId{0};

    // Position with linear interpolation between the last two daemon
    // updates. Daemon sends spawn updates at ~5 Hz; without lerp, dots
    // visibly teleport on each update. We lerp from the previous render
    // position to the latest target over the inter-update interval, so
    // motion looks continuous at the render rate.
    struct SmoothedPos {
        float  prevX{0}, prevY{0};
        float  targetX{0}, targetY{0};
        qint64 updateTimeMs{0};
        qint64 durationMs{0};   // 0 = snap to target (no animation)

        void snapTo(float x, float y, qint64 now) {
            prevX = targetX = x;
            prevY = targetY = y;
            updateTimeMs = now;
            durationMs = 0;
        }
        void retarget(float x, float y, qint64 now);
        QPointF posAt(qint64 now) const;
        bool settledAt(qint64 now) const {
            return durationMs <= 0 || (now - updateTimeMs) >= durationMs;
        }
    };

    // Spawns are rendered in drawForeground (batched via drawPoints) —
    // NOT as individual QGraphicsItems. With 5000+ spawns, individual
    // items with ItemIgnoresTransformations destroy frame rate (each
    // requires its own inverse-transform computation per paint). One
    // batched draw per color is much cheaper.
    struct SpawnRender {
        SmoothedPos        pos;
        seq::v1::SpawnType type{seq::v1::SPAWN_UNSPECIFIED};
        quint32            level{0};
    };
    QHash<quint32, SpawnRender> m_spawns;

    // Player position (smoothed) + heading. Heading is left unsmoothed —
    // angle wraparound makes naive lerp ugly and the visual artifact is
    // less obvious than position teleporting.
    bool        m_havePlayer{false};
    SmoothedPos m_playerPos;
    int         m_playerHeading{0};
    quint32     m_playerLevel{0};

    // Wall clock for interpolation timestamps + render-tick timer that
    // drives ~60 Hz repaints while anything is mid-lerp. Stops itself
    // once every spawn (and the player) has settled at its target so we
    // don't burn CPU on a static scene.
    QElapsedTimer m_animClock;
    QTimer*       m_renderTimer{nullptr};

    // Map geometry items (lines + locations) live in the scene because
    // they're static between zone changes. Tracked separately so
    // applyGeometry can wipe just these. Lines are batched into one
    // QGraphicsPathItem per color.
    QList<QGraphicsItem*> m_geometryItems;

    // Grid overlay (drawn in drawBackground so it sits under map lines).
    bool m_showGrid{true};

    // FPS overlay — ring buffer of recent inter-frame intervals (ms),
    // averaged for the on-screen readout. Sized for ~half-second window
    // at 60 FPS.
    bool                       m_showFps{true};
    QElapsedTimer              m_fpsTimer;
    std::array<qint64, 30>     m_frameTimes{};
    int                        m_frameTimesIdx{0};
    int                        m_frameTimesCount{0};

    // Diagnostic — last paintEvent / drawBackground / drawForeground
    // wall-clock durations in microseconds. Updated on each paint, shown
    // alongside the FPS readout.
    qint64 m_lastPaintUs{0};
    qint64 m_lastBackgroundUs{0};
    qint64 m_lastForegroundUs{0};

    // Pan state.
    QPoint  m_panStart;
    bool    m_panning{false};
};
