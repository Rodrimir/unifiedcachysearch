#ifndef PACKAGEPARSER_H
#define PACKAGEPARSER_H

#include <QList>
#include <QString>
#include <QStringList>
#include "core/searchitem.h"
#include "engine/commandbuilder.h"

// Conversao da saida textual de pacman/AUR helper/flatpak em SearchItem.
//
// Funcoes puras: nao executam processos nem tocam em estado global, o que as
// torna testaveis sem um SearchEngine (e sem a maquina ter pacman instalado).
namespace PackageParser {

// "gimp, inkscape" -> ["gimp", "inkscape"]
QStringList splitQuery(const QString &query);

QList<SearchItem> parsePacman(const QString &output, const CommandBuilder &commands);
QList<SearchItem> parseAur(const QString &output, const CommandBuilder &commands);
QList<SearchItem> parseFlatpak(const QString &output, const CommandBuilder &commands);

// Saida de `pacman -Si` / `<helper> -Si`.
SearchItem parseInfo(const QString &output, SearchItem::Repository type,
                     const CommandBuilder &commands);

QList<SearchItem> parsePacmanUpdates(const QString &output, const CommandBuilder &commands);
QList<SearchItem> parseAurUpdates(const QString &output, const CommandBuilder &commands);
QList<SearchItem> parseFlatpakUpdates(const QString &output, const CommandBuilder &commands);

int computeRelevance(const QString &name, const QString &description,
                     const QString &query);

QList<SearchItem> filterByRelevance(QList<SearchItem> items, const QString &query,
                                    int minScore = 15);

} // namespace PackageParser

#endif
