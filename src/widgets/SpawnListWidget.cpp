#include "SpawnListWidget.h"
#include "app/Settings.h"
#include "daemon/SpawnModel.h"
#include <QHeaderView>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>

SpawnListWidget::SpawnListWidget(SpawnModel* model, QWidget* parent)
    : QWidget(parent), m_model(model)
{
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("Filter spawns…");
    m_filterEdit->setClearButtonEnabled(true);

    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(SpawnModel::ColName);
    // Sort by EditRole so numeric columns (Dist, X, Y, Z, HP) sort
    // numerically; string-only columns (Name, Class) fall back to
    // DisplayRole automatically when EditRole returns invalid.
    m_proxy->setSortRole(Qt::EditRole);

    m_view = new QTreeView(this);
    m_view->setModel(m_proxy);
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);
    m_view->setSortingEnabled(true);
    m_view->sortByColumn(SpawnModel::ColName, Qt::AscendingOrder);
    // Interactive resize on every column so the user can drag any
    // edge — including Name. (Stretch mode locks the section width to
    // "fill remaining space" and disables dragging.) Give Name a
    // reasonable default width; the saved header state will override
    // it on subsequent launches.
    m_view->header()->setSectionResizeMode(QHeaderView::Interactive);
    m_view->header()->setStretchLastSection(true);
    m_view->setColumnWidth(SpawnModel::ColCon, 24);
    m_view->setColumnWidth(SpawnModel::ColName, 220);
    // Critical perf knob at high spawn counts: with uniform row heights
    // QTreeView's itemHeight/coordinateForItem become O(1) instead of
    // scanning every row. perf showed these two functions consuming 44%
    // of CPU at 5000 spawns before this was set.
    m_view->setUniformRowHeights(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    layout->addWidget(m_filterEdit);
    layout->addWidget(m_view);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &SpawnListWidget::onFilterChanged);
    connect(m_view, &QTreeView::doubleClicked, this, &SpawnListWidget::onRowDoubleClicked);
}

void SpawnListWidget::onRowDoubleClicked(const QModelIndex& proxyIndex) {
    if (!proxyIndex.isValid()) return;
    QModelIndex src = m_proxy->mapToSource(proxyIndex);
    quint32 id = m_model->spawnIdAt(src.row());
    if (id != 0)
        emit centerOnSpawn(id);
}

void SpawnListWidget::onFilterChanged(const QString& text) {
    m_proxy->setFilterFixedString(text);
    Settings::instance().setSpawnListFilter(text);
}

void SpawnListWidget::saveHeaderState() {
    Settings::instance().setSpawnListHeaderState(m_view->header()->saveState());
}

void SpawnListWidget::restoreHeaderState() {
    auto state = Settings::instance().spawnListHeaderState();
    if (!state.isEmpty())
        m_view->header()->restoreState(state);
    m_filterEdit->setText(Settings::instance().spawnListFilter());
}
