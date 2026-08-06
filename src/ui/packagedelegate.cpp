#include "ui/packagedelegate.h"
#include "core/repositoryinfo.h"
#include "core/searchmodel.h"
#include <QPainter>

namespace {
constexpr int Padding      = 8;
constexpr int BadgeWidth   = 60;
constexpr int RowHeight    = 62;
constexpr int TrailingArea = 80;

// Texto sobre os badges pastel: escuro nos dois temas, por contraste.
const QColor BadgeTextColor("#1e1e2e");
} // namespace

PackageDelegate::PackageDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void PackageDelegate::setThemeColors(bool dark)
{
    m_colors = dark ? darkTheme() : lightTheme();
}

void PackageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const bool isSelected = option.state & QStyle::State_Selected;
    const bool isHovered = option.state & QStyle::State_MouseOver;
    const QRect rect = option.rect;
    const QRect cardRect = rect.adjusted(4, 2, -4, -2);

    QColor bgColor(m_colors.bg1);
    if (isSelected)
        bgColor = QColor(m_colors.hoverBg);
    else if (isHovered)
        bgColor = QColor(m_colors.border);

    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(cardRect, 8, 8);

    if (isSelected) {
        painter->setPen(QPen(QColor(m_colors.accent), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(cardRect, 8, 8);
    }

    const auto repoType = static_cast<SearchItem::Repository>(
        index.data(SearchModel::RepositoryTypeRole).toInt());

    const QRect badgeRect(rect.left() + Padding + 4, rect.top() + 8, BadgeWidth, 22);
    painter->setBrush(QColor(RepositoryInfo::badgeColor(repoType)));
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(badgeRect, 11, 11);

    QFont badgeFont = option.font;
    badgeFont.setPixelSize(10);
    badgeFont.setBold(true);
    painter->setFont(badgeFont);
    painter->setPen(BadgeTextColor);
    painter->drawText(badgeRect, Qt::AlignCenter, RepositoryInfo::shortLabel(repoType));

    const int textLeft = rect.left() + Padding + BadgeWidth + 12;
    const int textWidth = rect.width() - BadgeWidth - Padding * 2 - TrailingArea;

    auto drawElided = [&](const QString &text, int top, int height) {
        const QRect box(textLeft, rect.top() + top, textWidth, height);
        painter->drawText(box, Qt::AlignLeft | Qt::AlignVCenter,
                          painter->fontMetrics().elidedText(text, Qt::ElideRight, box.width()));
    };

    QFont nameFont = option.font;
    nameFont.setPixelSize(13);
    nameFont.setBold(true);
    painter->setFont(nameFont);
    painter->setPen(QColor(m_colors.fg1));
    drawElided(index.data(SearchModel::NameRole).toString(), 6, 20);

    QFont smallFont = option.font;
    smallFont.setPixelSize(11);
    painter->setFont(smallFont);
    painter->setPen(QColor(m_colors.fg2));
    drawElided(index.data(SearchModel::VersionRole).toString(), 24, 16);

    const QString description = index.data(SearchModel::DescriptionRole).toString();
    if (!description.isEmpty()) {
        painter->setPen(QColor(m_colors.fg3));
        drawElided(description, 40, 16);
    }

    if (index.data(SearchModel::IsInstalledRole).toBool()) {
        const QRect installedRect(rect.right() - Padding - 70, rect.top() + 10, 65, 20);
        painter->setBrush(QColor(m_colors.green));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(installedRect, 10, 10);

        QFont installedFont = option.font;
        installedFont.setPixelSize(10);
        installedFont.setBold(true);
        painter->setFont(installedFont);
        painter->setPen(BadgeTextColor);
        painter->drawText(installedRect, Qt::AlignCenter, "Instalado");
    }

    painter->restore();
}

QSize PackageDelegate::sizeHint(const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QSize(option.rect.width(), RowHeight);
}
