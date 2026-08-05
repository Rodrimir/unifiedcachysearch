#include "core/sortfilterproxymodel.h"
#include "core/searchmodel.h"

SortFilterProxyModel::SortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortRole(SearchModel::NameRole);
}

void SortFilterProxyModel::setFilterPacman(bool enabled)
{
    beginResetModel();
    m_filterPacman = enabled;
    endResetModel();
}

void SortFilterProxyModel::setFilterAur(bool enabled)
{
    beginResetModel();
    m_filterAur = enabled;
    endResetModel();
}

void SortFilterProxyModel::setFilterFlatpak(bool enabled)
{
    beginResetModel();
    m_filterFlatpak = enabled;
    endResetModel();
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

    int sortCol = sortColumn();
    Q_UNUSED(sortCol)

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
