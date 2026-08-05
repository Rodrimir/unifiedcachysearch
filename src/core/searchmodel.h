#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include "core/searchitem.h"

class SearchModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        VersionRole,
        RepositoryRole,
        DisplayNameRole,
        InstallCommandRole,
        RepositoryTypeRole,
        DescriptionRole,
        IsInstalledRole,
        RepoBadgeColorRole,
        RelevanceScoreRole
    };

    explicit SearchModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addItem(const SearchItem &item);
    void addItems(const QList<SearchItem> &items);
    void setItemAt(int row, const SearchItem &item);
    void clear();
    const SearchItem &itemAt(int row) const;
    QList<SearchItem> allItems() const;

private:
    QList<SearchItem> m_items;
};

#endif
