#ifndef REPOSITORYINFO_H
#define REPOSITORYINFO_H

#include <QString>
#include "core/searchitem.h"

// Nomes, icones e cores por tipo de repositorio. Antes esse switch estava
// copiado no model, no delegate, no detail panel e no export.
namespace RepositoryInfo {

inline QString name(SearchItem::Repository type)
{
    switch (type) {
    case SearchItem::Pacman:  return QStringLiteral("Pacman");
    case SearchItem::AUR:     return QStringLiteral("AUR");
    case SearchItem::Flatpak: return QStringLiteral("Flatpak");
    }
    return {};
}

// Rotulo curto usado no badge da lista.
inline QString shortLabel(SearchItem::Repository type)
{
    switch (type) {
    case SearchItem::Pacman:  return QStringLiteral("Repo");
    case SearchItem::AUR:     return QStringLiteral("AUR");
    case SearchItem::Flatpak: return QStringLiteral("Flatpak");
    }
    return {};
}

inline QString icon(SearchItem::Repository type)
{
    switch (type) {
    case SearchItem::Pacman:  return QString::fromUtf8("\xF0\x9F\x93\xA6");
    case SearchItem::AUR:     return QString::fromUtf8("\xF0\x9F\x92\xA0");
    case SearchItem::Flatpak: return QString::fromUtf8("\xF0\x9F\x93\xA6");
    }
    return {};
}

inline QString badgeColor(SearchItem::Repository type)
{
    switch (type) {
    case SearchItem::Pacman:  return QStringLiteral("#89b4fa");
    case SearchItem::AUR:     return QStringLiteral("#f38ba8");
    case SearchItem::Flatpak: return QStringLiteral("#a6e3a1");
    }
    return {};
}

} // namespace RepositoryInfo

#endif
