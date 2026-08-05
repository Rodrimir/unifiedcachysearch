#ifndef PACKAGEDELEGATE_H
#define PACKAGEDELEGATE_H

#include <QStyledItemDelegate>

class PackageDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PackageDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

#endif
