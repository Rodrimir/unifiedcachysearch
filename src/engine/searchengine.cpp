#include "engine/searchengine.h"
#include "core/appsettings.h"
#include "engine/packageparser.h"
#include <QMutexLocker>
#include <QStandardPaths>

namespace {

// Timeouts fixos (nao configuraveis) das consultas auxiliares.
constexpr int InstalledListTimeoutMs   = 5000;
constexpr int PacmanInfoTimeoutMs      = 3000;
constexpr int AurInfoTimeoutMs         = 5000;
constexpr int PacmanUpdatesTimeoutMs   = 60000;
constexpr int AurUpdatesTimeoutMs      = 30000;
constexpr int FlatpakUpdatesTimeoutMs  = 30000;

// `pacman -Si` traduz os rotulos dos campos ("Name", "Version", ...) conforme
// o locale; sem forcar C o parser de detalhes nao encontra nada em sistemas
// que nao estao em ingles e o cross-reference fica sem efeito.
const QMap<QString, QString> &cLocaleEnv()
{
    static const QMap<QString, QString> env{
        {QStringLiteral("LC_ALL"), QStringLiteral("C")},
        {QStringLiteral("LANG"), QStringLiteral("C")},
    };
    return env;
}

QString detectAurHelper()
{
    for (const QString &helper : {QStringLiteral("paru"), QStringLiteral("yay")}) {
        if (!QStandardPaths::findExecutable(helper).isEmpty())
            return helper;
    }
    return {};
}

} // namespace

SearchEngine::SearchEngine(QObject *parent)
    : QObject(parent)
    , m_commands(detectAurHelper())
    , m_hasFlatpak(!QStandardPaths::findExecutable(QStringLiteral("flatpak")).isEmpty())
{
}

// --- Estado de instalacao -------------------------------------------------

void SearchEngine::invalidateInstalledCache()
{
    m_pkgCacheValid.storeRelease(0);
    m_flatpakCacheValid.storeRelease(0);
}

void SearchEngine::refreshInstalledPackages()
{
    QMutexLocker refreshLock(&m_refreshMutex);

    const auto result = m_runner.run(QStringLiteral("pacman"), {"-Qq"},
                                     InstalledListTimeoutMs);
    if (!result.succeeded())
        return; // falha transitoria: mantem o cache anterior

    QSet<QString> packages;
    const auto lines = result.standardOutput.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines)
        packages.insert(line.trimmed());

    {
        QMutexLocker lock(&m_pkgMutex);
        m_installedPkgs = std::move(packages);
    }
    m_pkgCacheValid.storeRelease(1);
}

void SearchEngine::refreshInstalledFlatpaks()
{
    if (!m_hasFlatpak)
        return;

    QMutexLocker refreshLock(&m_refreshMutex);

    const auto result = m_runner.run(QStringLiteral("flatpak"),
                                     {"list", "--columns=application"},
                                     InstalledListTimeoutMs);
    if (!result.succeeded())
        return;

    QSet<QString> apps;
    const auto lines = result.standardOutput.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString id = line.trimmed();
        if (!id.isEmpty() && id != "Application ID" && !id.contains('/'))
            apps.insert(id);
    }

    {
        QMutexLocker lock(&m_pkgMutex);
        m_installedFlatpaks = std::move(apps);
    }
    m_flatpakCacheValid.storeRelease(1);
}

void SearchEngine::ensureInstalledPackages()
{
    if (m_pkgCacheValid.loadAcquire())
        return;
    refreshInstalledPackages();
}

void SearchEngine::ensureInstalledFlatpaks()
{
    if (m_flatpakCacheValid.loadAcquire())
        return;
    refreshInstalledFlatpaks();
}

bool SearchEngine::isPackageInstalled(const QString &name) const
{
    QMutexLocker lock(&m_pkgMutex);
    return m_installedPkgs.contains(name);
}

bool SearchEngine::isFlatpakInstalled(const QString &name) const
{
    QMutexLocker lock(&m_pkgMutex);
    return m_installedFlatpaks.contains(name);
}

void SearchEngine::cancelSearch()
{
    m_runner.cancelAll();
}

// --- Buscas ---------------------------------------------------------------

QList<SearchItem> SearchEngine::searchPacman(const QString &query)
{
    const QStringList terms = PackageParser::splitQuery(query);
    if (terms.isEmpty())
        return {};

    ensureInstalledPackages();

    const auto result = m_runner.run(QStringLiteral("pacman"),
                                     QStringList{"-Ss"} + terms,
                                     AppSettings::pacmanTimeoutMs());
    if (!result.succeeded())
        return {};

    auto items = PackageParser::filterByRelevance(
        PackageParser::parsePacman(result.standardOutput, m_commands),
        terms.first(), AppSettings::minRelevanceScore());

    for (SearchItem &item : items)
        item.setIsInstalled(isPackageInstalled(item.name()));
    return items;
}

QList<SearchItem> SearchEngine::searchAur(const QString &query)
{
    if (!m_commands.hasAurHelper())
        return {};

    const QStringList terms = PackageParser::splitQuery(query);
    if (terms.isEmpty())
        return {};

    ensureInstalledPackages();

    const auto result = m_runner.run(m_commands.aurHelper(),
                                     QStringList{"-Ssa"} + terms,
                                     AppSettings::aurTimeoutMs());
    if (!result.succeeded())
        return {};

    auto items = PackageParser::filterByRelevance(
        PackageParser::parseAur(result.standardOutput, m_commands),
        terms.first(), AppSettings::minRelevanceScore());

    for (SearchItem &item : items)
        item.setIsInstalled(isPackageInstalled(item.name()));
    return items;
}

QList<SearchItem> SearchEngine::searchFlatpak(const QString &query)
{
    if (!m_hasFlatpak)
        return {};

    const QStringList terms = PackageParser::splitQuery(query);
    if (terms.isEmpty())
        return {};

    ensureInstalledFlatpaks();

    const auto result = m_runner.run(QStringLiteral("flatpak"),
                                     QStringList{"search"} + terms,
                                     AppSettings::flatpakTimeoutMs());
    if (!result.succeeded())
        return {};

    auto items = PackageParser::filterByRelevance(
        PackageParser::parseFlatpak(result.standardOutput, m_commands),
        terms.first(), AppSettings::minRelevanceScore());

    for (SearchItem &item : items)
        item.setIsInstalled(isFlatpakInstalled(item.name()));
    return items;
}

SearchItem SearchEngine::queryPacmanPackageInfo(const QString &name)
{
    const auto result = m_runner.run(QStringLiteral("pacman"), {"-Si", name},
                                     PacmanInfoTimeoutMs, cLocaleEnv());
    if (!result.succeeded())
        return {};

    SearchItem item = PackageParser::parseInfo(result.standardOutput,
                                               SearchItem::Pacman, m_commands);
    item.setIsInstalled(isPackageInstalled(name));
    return item;
}

SearchItem SearchEngine::queryAurPackageInfo(const QString &name)
{
    if (!m_commands.hasAurHelper())
        return {};

    const auto result = m_runner.run(m_commands.aurHelper(), {"-Si", name},
                                     AurInfoTimeoutMs, cLocaleEnv());
    if (!result.succeeded())
        return {};

    SearchItem item = PackageParser::parseInfo(result.standardOutput,
                                               SearchItem::AUR, m_commands);
    item.setIsInstalled(isPackageInstalled(name));
    return item;
}

// --- Atualizacoes ---------------------------------------------------------

QList<SearchItem> SearchEngine::checkPacmanUpdates()
{
    const auto result = m_runner.run(QStringLiteral("checkupdates"), {},
                                     PacmanUpdatesTimeoutMs);
    if (!result.succeeded())
        return {};
    return PackageParser::parsePacmanUpdates(result.standardOutput, m_commands);
}

QList<SearchItem> SearchEngine::checkAurUpdates()
{
    if (!m_commands.hasAurHelper())
        return {};

    const auto result = m_runner.run(m_commands.aurHelper(), {"-Qua"},
                                     AurUpdatesTimeoutMs);
    if (!result.succeeded())
        return {};
    return PackageParser::parseAurUpdates(result.standardOutput, m_commands);
}

QList<SearchItem> SearchEngine::checkFlatpakUpdates()
{
    if (!m_hasFlatpak)
        return {};

    const auto result = m_runner.run(QStringLiteral("flatpak"),
                                     {"remote-ls", "--updates"},
                                     FlatpakUpdatesTimeoutMs);
    if (!result.succeeded())
        return {};
    return PackageParser::parseFlatpakUpdates(result.standardOutput, m_commands);
}
