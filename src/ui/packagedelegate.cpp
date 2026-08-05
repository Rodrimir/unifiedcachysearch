#include "ui/packagedelegate.h"
#include "core/searchmodel.h"
#include <QPainter>

PackageDelegate::PackageDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void PackageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    bool isSelected = option.state & QStyle::State_Selected;
    bool isHovered = option.state & QStyle::State_MouseOver;

    QRect rect = option.rect;
    int padding = 8;
    int badgeWidth = 60;

    QColor bgColor = isSelected ? QColor("#45475a") : QColor("#1e1e2e");
    if (isHovered && !isSelected)
        bgColor = QColor("#313244");

    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(rect.adjusted(4, 2, -4, -2), 8, 8);

    if (isSelected) {
        painter->setPen(QPen(QColor("#89b4fa"), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rect.adjusted(4, 2, -4, -2), 8, 8);
    }

    QString repoColorStr = index.data(SearchModel::RepoBadgeColorRole).toString();
    QColor repoColor(repoColorStr);

    QRect badgeRect(rect.left() + padding + 4, rect.top() + 8, badgeWidth, 22);
    painter->setBrush(repoColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(badgeRect, 11, 11);

    QString repoType;
    switch (index.data(SearchModel::RepositoryTypeRole).toInt()) {
    case 0: repoType = "Repo"; break;
    case 1: repoType = "AUR"; break;
    case 2: repoType = "Flatpak"; break;
    }
    painter->setPen(QColor("#1e1e2e"));
    QFont badgeFont = option.font;
    badgeFont.setPixelSize(10);
    badgeFont.setBold(true);
    painter->setFont(badgeFont);
    painter->drawText(badgeRect, Qt::AlignCenter, repoType);

    QFont nameFont = option.font;
    nameFont.setPixelSize(13);
    nameFont.setBold(true);
    painter->setFont(nameFont);
    painter->setPen(isSelected ? QColor("#cdd6f4") : QColor("#cdd6f4"));

    QString name = index.data(SearchModel::NameRole).toString();
    QRect nameRect(rect.left() + padding + badgeWidth + 12, rect.top() + 6,
                   rect.width() - badgeWidth - padding * 2 - 80, 20);
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(name, Qt::ElideRight, nameRect.width()));

    QFont versionFont = option.font;
    versionFont.setPixelSize(11);
    painter->setFont(versionFont);
    painter->setPen(QColor("#a6adc8"));

    QString version = index.data(SearchModel::VersionRole).toString();
    QRect versionRect(rect.left() + padding + badgeWidth + 12, rect.top() + 24,
                      rect.width() - badgeWidth - padding * 2 - 80, 16);
    painter->drawText(versionRect, Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(version, Qt::ElideRight, versionRect.width()));

    QString desc = index.data(SearchModel::DescriptionRole).toString();
    if (!desc.isEmpty()) {
        QFont descFont = option.font;
        descFont.setPixelSize(11);
        painter->setFont(descFont);
        painter->setPen(QColor("#585b70"));
        QRect descRect(rect.left() + padding + badgeWidth + 12, rect.top() + 40,
                       rect.width() - badgeWidth - padding * 2 - 80, 16);
        painter->drawText(descRect, Qt::AlignLeft | Qt::AlignVCenter,
                          painter->fontMetrics().elidedText(desc, Qt::ElideRight, descRect.width()));
    }

    bool installed = index.data(SearchModel::IsInstalledRole).toBool();
    if (installed) {
        QRect installedRect(rect.right() - padding - 70, rect.top() + 10, 65, 20);
        painter->setBrush(QColor("#a6e3a1"));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(installedRect, 10, 10);
        painter->setPen(QColor("#1e1e2e"));
        QFont instFont = option.font;
        instFont.setPixelSize(10);
        instFont.setBold(true);
        painter->setFont(instFont);
        painter->drawText(installedRect, Qt::AlignCenter, "Instalado");
    }

    painter->restore();
}

QSize PackageDelegate::sizeHint(const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QSize(option.rect.width(), 62);
}
