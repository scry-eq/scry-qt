#pragma once
#include "seq/v1/events.pb.h"
#include <QAbstractTableModel>
#include <QHash>
#include <QVector>

struct SpawnRow {
    quint32 id{0};
    QString name;
    quint32 level{0};
    quint32 classId{0};
    quint32 raceId{0};
    quint32 hpCur{0};
    quint32 hpMax{0};
    float   x{0.0f};
    float   y{0.0f};
    float   z{0.0f};
    seq::v1::SpawnType type{seq::v1::SPAWN_UNSPECIFIED};
};

class SpawnModel : public QAbstractTableModel {
    Q_OBJECT
public:
    // ColCon is a con-color swatch, mirroring scry-web's leading dot
    // column. It has no text and never sorts.
    enum Column { ColCon, ColName, ColLevel, ColClass, ColRace, ColHP, ColDist,
                  ColX, ColY, ColZ, ColCount };

    explicit SpawnModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& = {}) const override;
    int columnCount(const QModelIndex& = {}) const override;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;

    quint32 spawnIdAt(int row) const;
    quint32 playerId() const { return m_playerId; }
    // 0 when the player isn't tracked or isn't in this zone's spawn set.
    quint32 playerLevel() const;

public slots:
    void clear();
    void applySnapshot(const seq::v1::Snapshot&);
    void applySpawnAdded(const seq::v1::SpawnAdded&);
    void applySpawnUpdated(const seq::v1::SpawnUpdated&);
    void applySpawnRemoved(quint32 id);

private slots:
    void flushDirtyRows();

private:
    static SpawnRow rowFromProto(const seq::v1::Spawn&);
    int indexOf(quint32 id) const;
    // Returns negative if no player tracked or player isn't in this zone.
    float distanceFromPlayer(int row) const;
    // Mark `row` for emit; coalesced via singleShot timer to avoid
    // firing one signal per spawn update at high spawn counts (5000
    // spawns × 5 Hz = 25000 signals/sec without coalescing).
    void scheduleRowDirty(int row);

    QVector<SpawnRow>      m_rows;
    QHash<quint32, int>    m_index; // id → row position
    quint32                m_playerId{0};

    // Coalesced repaint state.
    int  m_dirtyRowMin{INT_MAX};
    int  m_dirtyRowMax{INT_MIN};
    bool m_dirtyAllDistances{false};
    bool m_dirtyAllCons{false};
    bool m_flushScheduled{false};
};
