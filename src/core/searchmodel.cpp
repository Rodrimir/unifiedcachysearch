#include "core/searchmodel.h"

SearchModel::SearchModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex SearchModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int SearchModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

int SearchModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const SearchItem &item = m_items.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case DisplayNameRole:
        return item.displayName();
    case NameRole:
        return item.name();
    case VersionRole:
        return item.version();
    case RepositoryRole:
        return item.repository();
    case InstallCommandRole:
        return item.installCommand();
    case RepositoryTypeRole:
        return static_cast<int>(item.repositoryType());
    case DescriptionRole:
        return item.description();
    case IsInstalledRole:
        return item.isInstalled();
    case RepoBadgeColorRole: {
        switch (item.repositoryType()) {
        case SearchItem::Pacman:  return QString("#89b4fa");
        case SearchItem::AUR:     return QString("#f38ba8");
        case SearchItem::Flatpak: return QString("#a6e3a1");
        }
        return {};
    }
    case RelevanceScoreRole:
        return item.relevanceScore();
    default:
        return {};
    }
}

Qt::ItemFlags SearchModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> SearchModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {VersionRole, "version"},
        {RepositoryRole, "repository"},
        {DisplayNameRole, "displayName"},
        {InstallCommandRole, "installCommand"},
        {RepositoryTypeRole, "repositoryType"},
        {DescriptionRole, "description"},
        {IsInstalledRole, "isInstalled"},
        {RepoBadgeColorRole, "repoBadgeColor"},
        {RelevanceScoreRole, "relevanceScore"}
    };
}

void SearchModel::addItem(const SearchItem &item)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    endInsertRows();
}

void SearchModel::addItems(const QList<SearchItem> &items)
{
    if (items.isEmpty()) return;
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + items.size() - 1);
    m_items.append(items);
    endInsertRows();
}

void SearchModel::setItemAt(int row, const SearchItem &item)
{
    if (row < 0 || row >= m_items.size()) return;
    m_items[row] = item;
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {IsInstalledRole, DisplayNameRole});
}

void SearchModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

const SearchItem &SearchModel::itemAt(int row) const
{
    return m_items.at(row);
}

QList<SearchItem> SearchModel::allItems() const
{
    return m_items;
}
