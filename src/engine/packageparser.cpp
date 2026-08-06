#include "engine/packageparser.h"
#include <QRegularExpression>
#include <algorithm>

namespace {

// Termo de busca ja em minusculas com seu regex de limite de palavra
// pre-compilado: antes esse regex era construido para cada par
// (item, termo), ou seja, milhares de compilacoes por pesquisa.
struct Term {
    QString text;
    QRegularExpression wordBoundary;
};

QList<Term> compileTerms(const QString &query)
{
    static const QRegularExpression separators(QStringLiteral("[,\\s]+"));

    QList<Term> terms;
    const QStringList raw = query.toLower().split(separators, Qt::SkipEmptyParts);
    terms.reserve(raw.size());
    for (const QString &text : raw) {
        QRegularExpression boundary(
            QStringLiteral("(^|[^a-zA-Z0-9])") + QRegularExpression::escape(text)
            + QStringLiteral("($|[^a-zA-Z0-9])"));
        boundary.optimize();
        terms.append({text, boundary});
    }
    return terms;
}

int scoreTerms(const QList<Term> &terms, const QString &name, const QString &description)
{
    if (terms.isEmpty())
        return 1;

    const QString lowerName = name.toLower();
    const QString lowerDesc = description.toLower();

    int totalScore = 0;
    for (const Term &term : terms) {
        int termScore = 0;

        if (lowerName == term.text)
            termScore += 100;
        else if (lowerName.startsWith(term.text + '-') || lowerName.startsWith(term.text + '_'))
            termScore += 85;
        else if (lowerName.startsWith(term.text))
            termScore += 70;

        if (term.wordBoundary.match(lowerName).hasMatch())
            termScore = qMax(termScore, 65);
        else if (lowerName.contains(term.text))
            termScore = qMax(termScore, 40);

        if (term.wordBoundary.match(lowerDesc).hasMatch())
            termScore = qMax(termScore, 25);
        else if (lowerDesc.contains(term.text))
            termScore = qMax(termScore, 5);

        totalScore += termScore;
    }

    return totalScore / terms.size();
}

// A descricao de um pacote vem na linha seguinte, indentada.
QString descriptionAfter(const QStringList &lines, int index)
{
    if (index + 1 < lines.size() && lines[index + 1].startsWith(' '))
        return lines[index + 1].trimmed();
    return {};
}

} // namespace

namespace PackageParser {

QStringList splitQuery(const QString &query)
{
    QStringList terms;
    const QStringList raw = query.split(',', Qt::SkipEmptyParts);
    terms.reserve(raw.size());
    for (const QString &term : raw) {
        const QString trimmed = term.trimmed();
        if (!trimmed.isEmpty())
            terms << trimmed;
    }
    return terms;
}

QList<SearchItem> parsePacman(const QString &output, const CommandBuilder &commands)
{
    static const QRegularExpression pkgRegex(QStringLiteral(R"(^([^ /]+)/([^ ]+) +(.+)$)"));

    QList<SearchItem> results;
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];
        if (line.startsWith(' '))
            continue;

        const auto match = pkgRegex.match(line);
        if (!match.hasMatch())
            continue;

        const QString repo    = match.captured(1);
        const QString pkgName = match.captured(2);
        const QString version = match.captured(3);

        SearchItem item;
        item.setName(pkgName);
        item.setVersion(version);
        item.setRepository(repo);
        item.setDescription(descriptionAfter(lines, i));
        item.setRepositoryType(SearchItem::Pacman);
        item.setDisplayName(QString("[Repo: %1] %2 (%3)").arg(repo, pkgName, version));
        item.setInstallCommand(commands.installPacman({pkgName}));

        results.append(item);
    }
    return results;
}

QList<SearchItem> parseAur(const QString &output, const CommandBuilder &commands)
{
    QList<SearchItem> results;
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];
        if (!line.startsWith("aur/"))
            continue;

        const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 2)
            continue;

        const QString pkgName = parts[0].section('/', 1, 1);
        const QString version = parts[1];

        SearchItem item;
        item.setName(pkgName);
        item.setVersion(version);
        item.setRepository(QStringLiteral("aur"));
        item.setDescription(descriptionAfter(lines, i));
        item.setRepositoryType(SearchItem::AUR);
        item.setDisplayName(QString("[AUR] %1 (%2)").arg(pkgName, version));
        item.setInstallCommand(commands.installAur({pkgName}));

        results.append(item);
    }
    return results;
}

QList<SearchItem> parseFlatpak(const QString &output, const CommandBuilder &commands)
{
    QList<SearchItem> results;
    if (output.contains("No matches"))
        return results;

    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty())
            continue;

        // KeepEmptyParts preserva o alinhamento das colunas: com
        // SkipEmptyParts um pacote sem descricao deslocava todos os campos e
        // acabava descartado.
        const QStringList parts = line.split('\t', Qt::KeepEmptyParts);
        if (parts.size() < 3)
            continue;

        const QString appId = parts[2].trimmed();
        // Descarta o cabecalho ("Application ID") e linhas invalidas.
        if (appId.isEmpty() || !appId.contains('.'))
            continue;

        const QString friendlyName = parts[0].trimmed();
        const QString description  = parts[1].trimmed();
        const QString version      = parts.size() > 3 ? parts[3].trimmed() : QString();
        const QString remote       = parts.size() > 5 && !parts[5].trimmed().isEmpty()
            ? parts[5].trimmed()
            : QStringLiteral("flathub");

        SearchItem item;
        item.setName(appId);
        item.setVersion(version);
        item.setRepository(remote);
        item.setDescription(description);
        item.setRepositoryType(SearchItem::Flatpak);
        item.setDisplayName(QString("[Flatpak] %1 (%2)").arg(friendlyName, version));
        item.setInstallCommand(commands.installFlatpak(remote, {appId}));

        results.append(item);
    }
    return results;
}

SearchItem parseInfo(const QString &output, SearchItem::Repository type,
                     const CommandBuilder &commands)
{
    SearchItem item;
    item.setRepositoryType(type);

    const QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        if (line.startsWith("Name"))
            item.setName(line.section(':', 1).trimmed());
        else if (line.startsWith("Version"))
            item.setVersion(line.section(':', 1).trimmed());
        else if (line.startsWith("Repository"))
            item.setRepository(line.section(':', 1).trimmed());
        else if (line.startsWith("Description"))
            item.setDescription(line.section(':', 1).trimmed());
    }

    if (item.name().isEmpty())
        return item;

    item.setRelevanceScore(100);

    QString prefix;
    switch (type) {
    case SearchItem::Pacman:
        prefix = QString("[Repo: %1]").arg(item.repository());
        item.setInstallCommand(commands.installPacman({item.name()}));
        break;
    case SearchItem::AUR:
        prefix = QStringLiteral("[AUR]");
        item.setInstallCommand(commands.installAur({item.name()}));
        break;
    case SearchItem::Flatpak:
        prefix = QStringLiteral("[Flatpak]");
        item.setInstallCommand(commands.installFlatpak(item.repository(), {item.name()}));
        break;
    }
    item.setDisplayName(QString("%1 %2 (%3)").arg(prefix, item.name(), item.version()));
    return item;
}

QList<SearchItem> parsePacmanUpdates(const QString &output, const CommandBuilder &commands)
{
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));

    QList<SearchItem> results;
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        // Formato do checkupdates: "nome versao-antiga -> versao-nova"
        const QStringList parts = line.trimmed().split(whitespace, Qt::SkipEmptyParts);
        if (parts.size() < 4 || parts[2] != "->" || parts[1] == parts[3])
            continue;

        SearchItem item;
        item.setName(parts[0]);
        item.setVersion(QString("%1 → %2").arg(parts[1], parts[3]));
        item.setRepository(QStringLiteral("pacman"));
        item.setRepositoryType(SearchItem::Pacman);
        item.setDisplayName(QString("[Repo] %1 (%2 → %3)").arg(parts[0], parts[1], parts[3]));
        item.setInstallCommand(commands.installPacman({parts[0]}));
        item.setIsInstalled(true);
        results.append(item);
    }
    return results;
}

QList<SearchItem> parseAurUpdates(const QString &output, const CommandBuilder &commands)
{
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));

    QList<SearchItem> results;
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        const QStringList parts = line.trimmed().split(whitespace, Qt::SkipEmptyParts);
        if (parts.size() < 4 || parts[2] != "->" || parts[1] == parts[3])
            continue;

        SearchItem item;
        item.setName(parts[0]);
        item.setVersion(QString("%1 → %2").arg(parts[1], parts[3]));
        item.setRepository(QStringLiteral("aur"));
        item.setRepositoryType(SearchItem::AUR);
        item.setDisplayName(QString("[AUR] %1 (%2 → %3)").arg(parts[0], parts[1], parts[3]));
        item.setInstallCommand(commands.installAur({parts[0]}));
        item.setIsInstalled(true);
        results.append(item);
    }
    return results;
}

QList<SearchItem> parseFlatpakUpdates(const QString &output, const CommandBuilder &commands)
{
    QList<SearchItem> results;
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        const QStringList parts = line.trimmed().split('\t', Qt::KeepEmptyParts);
        if (parts.size() < 5)
            continue;

        const QString appId = parts[1].trimmed();
        if (!appId.contains('.'))
            continue;

        const QString version = parts[3].trimmed();

        SearchItem item;
        item.setName(appId);
        item.setVersion(version);
        item.setRepository(parts[4].trimmed());
        item.setRepositoryType(SearchItem::Flatpak);
        item.setDisplayName(QString("[Flatpak] %1 (%2)").arg(appId, version));
        item.setInstallCommand(commands.updateFlatpak({appId}));
        item.setIsInstalled(true);
        results.append(item);
    }
    return results;
}

int computeRelevance(const QString &name, const QString &description,
                     const QString &query)
{
    if (query.isEmpty())
        return 1;
    return scoreTerms(compileTerms(query), name, description);
}

QList<SearchItem> filterByRelevance(QList<SearchItem> items, const QString &query,
                                    int minScore)
{
    // Termos compilados uma unica vez para toda a lista.
    const QList<Term> terms = compileTerms(query);

    for (SearchItem &item : items)
        item.setRelevanceScore(query.isEmpty()
            ? 1
            : scoreTerms(terms, item.name(), item.description()));

    items.erase(std::remove_if(items.begin(), items.end(),
                               [minScore](const SearchItem &i) {
                                   return i.relevanceScore() < minScore;
                               }),
                items.end());

    std::sort(items.begin(), items.end(),
              [](const SearchItem &a, const SearchItem &b) {
                  if (a.relevanceScore() != b.relevanceScore())
                      return a.relevanceScore() > b.relevanceScore();
                  return a.name().toLower() < b.name().toLower();
              });

    return items;
}

} // namespace PackageParser
