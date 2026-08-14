#include "Settings.h"
#include <QColor>

static constexpr char kDaemonUrl[]             = "daemon/url";
static constexpr char kDaemonHistory[]          = "daemon/history";
static constexpr char kMainWindowGeometry[]    = "mainwindow/geometry";
static constexpr char kMainWindowState[]       = "mainwindow/state";
static constexpr char kSpawnListHeaderState[]  = "spawnlist/headerState";
static constexpr char kSpawnListFilter[]       = "spawnlist/filter";
static constexpr char kMapBackground[]         = "map/bgcolor";

Settings& Settings::instance() {
    static Settings s;
    return s;
}

Settings::Settings()
    : m_settings("scry-eq", "scry-qt")
{}

QUrl Settings::daemonUrl() const {
    return QUrl(m_settings.value(kDaemonUrl, "ws://127.0.0.1:9090").toString());
}
void Settings::setDaemonUrl(const QUrl& url) {
    m_settings.setValue(kDaemonUrl, url.toString());
}

QStringList Settings::daemonHistory() const {
    return m_settings.value(kDaemonHistory).toStringList();
}

void Settings::addDaemonHistory(const QUrl& url) {
    QString s = url.toString().trimmed();
    if (s.isEmpty()) return;
    QStringList hist = daemonHistory();
    hist.removeAll(s);          // dedupe
    hist.prepend(s);            // most-recent first
    while (hist.size() > kHistoryMax) hist.removeLast();
    m_settings.setValue(kDaemonHistory, hist);
}

QByteArray Settings::mainWindowGeometry() const {
    return m_settings.value(kMainWindowGeometry).toByteArray();
}
void Settings::setMainWindowGeometry(const QByteArray& v) {
    m_settings.setValue(kMainWindowGeometry, v);
}

QByteArray Settings::mainWindowState() const {
    return m_settings.value(kMainWindowState).toByteArray();
}
void Settings::setMainWindowState(const QByteArray& v) {
    m_settings.setValue(kMainWindowState, v);
}

QByteArray Settings::spawnListHeaderState() const {
    return m_settings.value(kSpawnListHeaderState).toByteArray();
}
void Settings::setSpawnListHeaderState(const QByteArray& v) {
    m_settings.setValue(kSpawnListHeaderState, v);
}

QString Settings::spawnListFilter() const {
    return m_settings.value(kSpawnListFilter).toString();
}
void Settings::setSpawnListFilter(const QString& v) {
    m_settings.setValue(kSpawnListFilter, v);
}

QColor Settings::mapBackground() const {
    return QColor(m_settings.value(kMapBackground, "#1a1a1a").toString());
}
void Settings::setMapBackground(const QColor& c) {
    m_settings.setValue(kMapBackground, c.name());
}
