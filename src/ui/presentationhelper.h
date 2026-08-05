#ifndef PRESENTATIONHELPER_H
#define PRESENTATIONHELPER_H

#include <QString>
#include "core/searchitem.h"

namespace PresentationHelper {

inline QString repoIcon(SearchItem::Repository type)
{
    switch (type) {
    case SearchItem::Pacman:  return QString::fromUtf8("\xF0\x9F\x93\xA6");
    case SearchItem::AUR:     return QString::fromUtf8("\xF0\x9F\x92\xA0");
    case SearchItem::Flatpak: return QString::fromUtf8("\xF0\x9F\x93\xA6");
    }
    return {};
}

inline QString repoBadgeColor(SearchItem::Repository type)
{
    switch (type) {
    case SearchItem::Pacman:  return "#89b4fa";
    case SearchItem::AUR:     return "#f38ba8";
    case SearchItem::Flatpak: return "#a6e3a1";
    }
    return {};
}

} // namespace PresentationHelper

#endif
