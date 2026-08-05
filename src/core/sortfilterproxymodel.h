#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "core/searchitem.h"

class SortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit SortFilterProxyModel(QObject *parent = nullptr);

    void setFilterPacman(bool enabled);
    void setFilterAur(bool enabled);
    void setFilterFlatpak(bool enabled);

    bool filterPacman() const;
    bool filterAur() const;
    bool filterFlatpak() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    bool m_filterPacman = true;
    bool m_filterAur = true;
    bool m_filterFlatpak = true;
};

#endif
