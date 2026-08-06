#ifndef PACKAGEDELEGATE_H
#define PACKAGEDELEGATE_H

#include <QStyledItemDelegate>
#include "core/theme.h"

class PackageDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit PackageDelegate(QObject *parent = nullptr);

    // As cores eram fixas no tema escuro: no tema claro a lista continuava
    // preta enquanto o resto da janela clareava.
    void setThemeColors(bool dark);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    ThemeColors m_colors = darkTheme();
};

#endif
