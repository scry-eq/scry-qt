#include "SpawnModel.h"
#include "daemon/ZoneState.h"
#include "util/ConColor.h"
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QTimer>
#include <climits>
#include <cmath>

static const char* kClassNames[] = {
    "Unknown","Warrior","Cleric","Paladin","Ranger","SK","Druid",
    "Monk","Bard","Rogue","Shaman","Necro","Wiz","Mage","Enc","BST","Ber"
};
static const int kClassCount = static_cast<int>(sizeof(kClassNames)/sizeof(kClassNames[0]));

// Race-name table from scry-cpp/src/races.h (mirrored verbatim).
// Sparse: indices without an entry default to NULL → fall back to a
// numeric string in raceName().
static const char* kRaceNames[] = {
#include "daemon/races.h"
};
static const int kRaceCount = static_cast<int>(sizeof(kRaceNames)/sizeof(kRaceNames[0]));

static QString raceName(uint32_t race) {
    if (race < static_cast<uint32_t>(kRaceCount) && kRaceNames[race])
        return QString::fromLatin1(kRaceNames[race]);
    return QString::number(race);
}

// Con swatch, mirroring scry-web's 10px ring-bordered dot. Cached per
// band — data() is called for every visible cell on every repaint, and
// painting a fresh pixmap each time would be pure waste.
// QImage rather than QPixmap: the cache outlives QApplication, and a
// QPixmap destroyed after GUI teardown is a known Qt footgun.
static QImage conDot(Con c) {
    static QHash<int, QImage> cache;
    const int key = static_cast<int>(c);
    auto it = cache.constFind(key);
    if (it != cache.constEnd())
        return it.value();

    QImage pm(10, 10, QImage::Format_ARGB32_Premultiplied);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(conColor(c));
    p.setPen(QPen(QColor(0xff, 0xff, 0xff, 0x40), 1));
    p.drawEllipse(QRectF(0.5, 0.5, 9.0, 9.0));
    p.end();
    cache.insert(key, pm);
    return pm;
}

SpawnModel::SpawnModel(QObject* parent) : QAbstractTableModel(parent) {}

quint32 SpawnModel::playerLevel() const {
    if (m_playerId == 0) return 0;
    const int row = indexOf(m_playerId);
    return row < 0 ? 0 : m_rows[row].level;
}

int SpawnModel::rowCount(const QModelIndex&) const { return m_rows.size(); }
int SpawnModel::columnCount(const QModelIndex&) const { return ColCount; }

QVariant SpawnModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColCon:   return "";
    case ColName:  return "Name";
    case ColLevel: return "Lvl";
    case ColClass: return "Class";
    case ColRace:  return "Race";
    case ColHP:    return "HP%";
    case ColDist:  return "Dist";
    case ColX:     return "X";
    case ColY:     return "Y";
    case ColZ:     return "Z";
    }
    return {};
}

QVariant SpawnModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const SpawnRow& r = m_rows[index.row()];

    // EditRole carries the raw numeric value so the proxy sorts
    // numerically (set sortRole = EditRole on the proxy).
    if (role == Qt::EditRole) {
        switch (index.column()) {
        // Sorts grey -> red, i.e. ascending threat.
        case ColCon:   return static_cast<int>(
                           conOf(static_cast<int>(playerLevel()),
                                 static_cast<int>(r.level)));
        case ColLevel: return r.level;
        case ColHP:    return r.hpMax > 0 ? (100.0 * r.hpCur / r.hpMax) : -1.0;
        case ColDist:  {
            float d = distanceFromPlayer(index.row());
            return d < 0 ? QVariant() : QVariant(d);
        }
        case ColX: return r.x;
        case ColY: return r.y;
        case ColZ: return r.z;
        }
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:  return r.name;
        case ColLevel: return r.level;
        case ColClass: {
            int c = static_cast<int>(r.classId);
            return (c > 0 && c < kClassCount) ? QString(kClassNames[c]) : QString::number(c);
        }
        case ColRace:  return raceName(r.raceId);
        case ColHP:
            return r.hpMax > 0
                ? QString::number(100u * r.hpCur / r.hpMax) + "%"
                : QString("?");
        case ColDist: {
            float d = distanceFromPlayer(index.row());
            return d < 0 ? QString("—") : QString::number(d, 'f', 0);
        }
        case ColX:     return QString::number(r.x, 'f', 1);
        case ColY:     return QString::number(r.y, 'f', 1);
        case ColZ:     return QString::number(r.z, 'f', 1);
        }
    }
    // Con swatch. Matches scry-web, where the leading dot carries the
    // con and the row text stays neutral — type is read off the Class
    // column and the map glyph, not the text color.
    if (role == Qt::DecorationRole && index.column() == ColCon)
        return conDot(conOf(static_cast<int>(playerLevel()),
                            static_cast<int>(r.level)));

    if (role == Qt::ForegroundRole) {
        // Corpses stay dimmed so they read as objects rather than threats
        // (the map draws them as hollow outline glyphs for the same reason).
        switch (r.type) {
        case seq::v1::CORPSE_PC:  return QColor(Qt::gray);
        case seq::v1::CORPSE_NPC: return QColor(Qt::darkGray);
        default: break;
        }
        // Con-colored row text, as legacy showeq does it
        // (spawnlistcommon.cpp: m_textColor = pickConColor). Legacy also
        // darkens yellow and swaps white for the default text color, both
        // for light backgrounds; this list is dark, so the palette stands.
        return conColorOf(static_cast<int>(playerLevel()),
                          static_cast<int>(r.level));
    }
    return {};
}

quint32 SpawnModel::spawnIdAt(int row) const {
    if (row < 0 || row >= m_rows.size()) return 0;
    return m_rows[row].id;
}

void SpawnModel::clear() {
    beginResetModel();
    m_rows.clear();
    m_index.clear();
    endResetModel();
}

SpawnRow SpawnModel::rowFromProto(const seq::v1::Spawn& s) {
    SpawnRow r;
    r.id      = s.id();
    r.name    = QString::fromStdString(s.name());
    r.level   = s.level();
    r.classId = s.class_();
    r.raceId  = s.race();
    r.hpCur   = s.hp_cur();
    r.hpMax   = s.hp_max();
    r.type    = s.type();
    if (s.has_pos()) {
        r.x = ZoneState::toWorld(s.pos().x());
        r.y = ZoneState::toWorld(s.pos().y());
        r.z = ZoneState::toWorld(s.pos().z());
    }
    return r;
}

void SpawnModel::applySnapshot(const seq::v1::Snapshot& snap) {
    beginResetModel();
    m_rows.clear();
    m_index.clear();
    m_playerId = snap.player_id();
    for (const auto& s : snap.spawns()) {
        m_index[s.id()] = m_rows.size();
        m_rows.push_back(rowFromProto(s));
    }
    endResetModel();
}

void SpawnModel::applySpawnAdded(const seq::v1::SpawnAdded& msg) {
    const auto& s = msg.spawn();
    if (m_index.contains(s.id()))
        return; // shouldn't happen, but guard duplicates
    beginInsertRows({}, m_rows.size(), m_rows.size());
    m_index[s.id()] = m_rows.size();
    m_rows.push_back(rowFromProto(s));
    endInsertRows();
}

void SpawnModel::applySpawnUpdated(const seq::v1::SpawnUpdated& msg) {
    int row = indexOf(msg.id());
    if (row < 0) return;
    SpawnRow& r = m_rows[row];
    if (msg.has_pos()) {
        r.x = ZoneState::toWorld(msg.pos().x());
        r.y = ZoneState::toWorld(msg.pos().y());
        r.z = ZoneState::toWorld(msg.pos().z());
    }
    if (msg.has_hp_cur()) r.hpCur = msg.hp_cur();
    if (msg.has_level())  r.level = msg.level();
    if (msg.has_name())   r.name  = QString::fromStdString(msg.name());

    scheduleRowDirty(row);
    if (msg.id() == m_playerId) {
        // Player moved → every other row's distance is now stale.
        if (msg.has_pos())
            m_dirtyAllDistances = true;
        // Player dinged → every other row's con is now stale.
        if (msg.has_level())
            m_dirtyAllCons = true;
    }
}

void SpawnModel::applySpawnRemoved(quint32 id) {
    int row = indexOf(id);
    if (row < 0) return;
    beginRemoveRows({}, row, row);
    m_index.remove(id);
    m_rows.remove(row);
    // Rebuild index for rows after the removed one.
    for (int i = row; i < m_rows.size(); ++i)
        m_index[m_rows[i].id] = i;
    endRemoveRows();
}

int SpawnModel::indexOf(quint32 id) const {
    auto it = m_index.find(id);
    return it != m_index.end() ? it.value() : -1;
}

float SpawnModel::distanceFromPlayer(int row) const {
    if (m_playerId == 0 || row < 0 || row >= m_rows.size()) return -1.0f;
    int playerRow = indexOf(m_playerId);
    if (playerRow < 0 || playerRow == row) return -1.0f;
    const SpawnRow& p = m_rows[playerRow];
    const SpawnRow& s = m_rows[row];
    float dx = s.x - p.x, dy = s.y - p.y, dz = s.z - p.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void SpawnModel::scheduleRowDirty(int row) {
    if (row < 0) return;
    m_dirtyRowMin = std::min(m_dirtyRowMin, row);
    m_dirtyRowMax = std::max(m_dirtyRowMax, row);
    if (!m_flushScheduled) {
        m_flushScheduled = true;
        QTimer::singleShot(50, this, &SpawnModel::flushDirtyRows);
    }
}

// 20 Hz flush — emits at most one dataChanged per coalescing window
// covering the full set of dirty rows. Keeps the QTreeView happy at
// 5000+ rows × 5 Hz spawn updates.
void SpawnModel::flushDirtyRows() {
    m_flushScheduled = false;
    if (m_dirtyRowMin <= m_dirtyRowMax) {
        const int lo = std::max(0, m_dirtyRowMin);
        const int hi = std::min(static_cast<int>(m_rows.size()) - 1, m_dirtyRowMax);
        if (lo <= hi)
            emit dataChanged(index(lo, 0), index(hi, ColCount - 1));
        m_dirtyRowMin = INT_MAX;
        m_dirtyRowMax = INT_MIN;
    }
    if (m_dirtyAllDistances && !m_rows.isEmpty()) {
        emit dataChanged(index(0, ColDist),
                         index(m_rows.size() - 1, ColDist));
        m_dirtyAllDistances = false;
    }
    // A ding re-cons every spawn, and con drives the whole row's text
    // color — not just the swatch — so this spans all columns.
    if (m_dirtyAllCons && !m_rows.isEmpty()) {
        emit dataChanged(index(0, 0),
                         index(m_rows.size() - 1, ColCount - 1));
        m_dirtyAllCons = false;
    }
}
