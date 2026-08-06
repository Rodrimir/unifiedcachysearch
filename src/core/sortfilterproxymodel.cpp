#include "core/sortfilterproxymodel.h"
#include "core/searchmodel.h"

// Reaplica o filtro sem resetar o modelo (o reset descartava selecao e
// scroll). invalidateFilter() virou deprecated no Qt 6.9.
void SortFilterProxyModel::reapplyFilter()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    beginFilterChange();
    endFilterChange(Direction::Rows);
#else
    invalidateFilter();
#endif
}

SortFilterProxyModel::SortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortRole(SearchModel::NameRole);
}

// invalidateFilter() e a API correta: reavalia filterAcceptsRow sem destruir a
// selecao e o scroll como um reset de modelo faria.
void SortFilterProxyModel::setFilterPacman(bool enabled)
{
    if (m_filterPacman == enabled) return;
    m_filterPacman = enabled;
    reapplyFilter();
}

void SortFilterProxyModel::setFilterAur(bool enabled)
{
    if (m_filterAur == enabled) return;
    m_filterAur = enabled;
    reapplyFilter();
}

void SortFilterProxyModel::setFilterFlatpak(bool enabled)
{
    if (m_filterFlatpak == enabled) return;
    m_filterFlatpak = enabled;
    reapplyFilter();
}

bool SortFilterProxyModel::filterPacman() const { return m_filterPacman; }
bool SortFilterProxyModel::filterAur() const { return m_filterAur; }
bool SortFilterProxyModel::filterFlatpak() const { return m_filterFlatpak; }

bool SortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    auto *model = sourceModel();
    if (!model) return true;

    QModelIndex idx = model->index(sourceRow, 0, sourceParent);
    int type = model->data(idx, SearchModel::RepositoryTypeRole).toInt();

    switch (type) {
    case SearchItem::Pacman:  return m_filterPacman;
    case SearchItem::AUR:     return m_filterAur;
    case SearchItem::Flatpak: return m_filterFlatpak;
    }
    return true;
}

bool SortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto *model = sourceModel();
    if (!model) return false;

    int leftScore = model->data(left, SearchModel::RelevanceScoreRole).toInt();
    int rightScore = model->data(right, SearchModel::RelevanceScoreRole).toInt();

    if (leftScore != rightScore)
        return leftScore > rightScore;

    QString leftName = model->data(left, SearchModel::NameRole).toString();
    QString rightName = model->data(right, SearchModel::NameRole).toString();

    int leftType = model->data(left, SearchModel::RepositoryTypeRole).toInt();
    int rightType = model->data(right, SearchModel::RepositoryTypeRole).toInt();

    if (leftType != rightType)
        return leftType < rightType;

    return QString::localeAwareCompare(leftName, rightName) < 0;
}
