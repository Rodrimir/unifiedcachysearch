#include "engine/searchengine.h"
#include <QRegularExpression>
#include <QMutexLocker>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <algorithm>

SearchEngine::SearchEngine(QObject *parent)
    : QObject(parent)
    , m_aurHelper(detectAurHelper())
    , m_hasFlatpak(checkFlatpak())
{
}

QString SearchEngine::detectAurHelper()
{
    QProcess proc;
    proc.start("which", {"paru"});
    proc.waitForFinished(3000);
    if (proc.exitCode() == 0 && !proc.readAllStandardOutput().isEmpty())
        return "paru";

    proc.start("which", {"yay"});
    proc.waitForFinished(3000);
    if (proc.exitCode() == 0 && !proc.readAllStandardOutput().isEmpty())
        return "yay";

    return {};
}

bool SearchEngine::checkFlatpak()
{
    QProcess proc;
    proc.start("which", {"flatpak"});
    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}

QString SearchEngine::aurHelper() const { return m_aurHelper; }
bool SearchEngine::hasFlatpak() const { return m_hasFlatpak; }

QString SearchEngine::runProcess(const QString &bin, const QStringList &args, int timeoutMs)
{
    auto *proc = new QProcess;
    {
        QMutexLocker lock(&m_processMutex);
        m_activeProcesses.append(proc);
    }

    proc->start(bin, args);
    proc->waitForFinished(timeoutMs);
    QString output = (proc->exitCode() == 0)
        ? QString::fromUtf8(proc->readAllStandardOutput())
        : QString();

    {
        QMutexLocker lock(&m_processMutex);
        m_activeProcesses.removeOne(proc);
    }
    delete proc;

    return output;
}

void SearchEngine::refreshInstalledPkgs()
{
    QString output = runProcess("pacman", {"-Qq"}, 5000);
    QMutexLocker lock(&m_pkgMutex);
    m_installedPkgs.clear();
    if (!output.isEmpty()) {
        const auto lines = output.split('\n', Qt::SkipEmptyParts);
        for (const auto &line : lines)
            m_installedPkgs.insert(line.trimmed());
    }
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

void SearchEngine::refreshInstalledFlatpaks()
{
    QString output = runProcess("flatpak", {"list", "--columns=application"}, 5000);
    QMutexLocker lock(&m_pkgMutex);
    m_installedFlatpaks.clear();
    if (!output.isEmpty()) {
        const auto lines = output.split('\n', Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            QString id = line.trimmed();
            if (!id.isEmpty() && id != "Application ID" && !id.contains('/'))
                m_installedFlatpaks.insert(id);
        }
    }
}

void SearchEngine::cancelSearch()
{
    QMutexLocker lock(&m_processMutex);
    for (auto *p : m_activeProcesses) {
        if (p->state() != QProcess::NotRunning)
            p->kill();
    }
    m_activeProcesses.clear();
}

QStringList SearchEngine::splitQuery(const QString &query)
{
    QStringList terms = query.split(',', Qt::SkipEmptyParts);
    for (auto &t : terms)
        t = t.trimmed();
    if (terms.isEmpty())
        terms << query.trimmed();
    return terms;
}

QList<SearchItem> SearchEngine::parsePacmanOutput(const QString &output)
{
    QList<SearchItem> results;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QRegularExpression pkgRegex(R"(^([^ /]+)/([^ ]+) +(.+)$)");

    for (int i = 0; i < lines.size(); ++i) {
        const auto &line = lines[i];
        if (line.startsWith(' '))
            continue;

        auto match = pkgRegex.match(line);
        if (!match.hasMatch())
            continue;

        QString repo = match.captured(1);
        QString pkgName = match.captured(2);
        QString version = match.captured(3);

        QString description;
        if (i + 1 < lines.size() && lines[i + 1].startsWith(' '))
            description = lines[i + 1].trimmed();

        SearchItem item;
        item.setName(pkgName);
        item.setVersion(version);
        item.setRepository(repo);
        item.setDescription(description);
        item.setRepositoryType(SearchItem::Pacman);
        item.setDisplayName(QString("[Repo: %1] %2 (%3)").arg(repo, pkgName, version));
        item.setInstallCommand(pacmanInstallCmd(pkgName));

        results.append(item);
    }
    return results;
}

QList<SearchItem> SearchEngine::parseAurOutput(const QString &output)
{
    QList<SearchItem> results;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (int i = 0; i < lines.size(); ++i) {
        const auto &line = lines[i];
        if (!line.startsWith("aur/"))
            continue;

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 2)
            continue;

        QString pkgName = parts[0].section('/', 1, 1);
        QString version = parts[1];

        QString description;
        if (i + 1 < lines.size() && lines[i + 1].startsWith(' '))
            description = lines[i + 1].trimmed();

        SearchItem item;
        item.setName(pkgName);
        item.setVersion(version);
        item.setRepository("aur");
        item.setDescription(description);
        item.setRepositoryType(SearchItem::AUR);
        item.setDisplayName(QString("[AUR] %1 (%2)").arg(pkgName, version));
        item.setInstallCommand(aurInstallCmd(m_aurHelper, pkgName));

        results.append(item);
    }
    return results;
}

QList<SearchItem> SearchEngine::parseFlatpakOutput(const QString &output)
{
    QList<SearchItem> results;
    if (output.contains("No matches"))
        return results;

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (int i = 1; i < lines.size(); ++i) {
        const QString &line = lines[i].trimmed();
        if (line.isEmpty())
            continue;

        QStringList parts = line.split('\t', Qt::SkipEmptyParts);
        if (parts.size() < 2)
            continue;

        QString friendlyName = parts[0];
        QString description  = parts.size() > 1 ? parts[1] : QString();
        QString appId        = parts.size() > 2 ? parts[2] : QString();
        if (appId.isEmpty() || !appId.contains('.'))
            continue;

        QString version = parts.size() > 3 ? parts[3] : QString();
        QString remote  = parts.size() > 5 ? parts[5] : "flathub";

        SearchItem item;
        item.setName(appId);
        item.setVersion(version);
        item.setRepository(remote);
        item.setDescription(description);
        item.setRepositoryType(SearchItem::Flatpak);
        item.setDisplayName(QString("[Flatpak] %1 (%2)").arg(friendlyName, version));
        item.setInstallCommand(flatpakInstallCmd(remote, appId));

        results.append(item);
    }
    return results;
}

QList<SearchItem> SearchEngine::searchPacman(const QString &query)
{
    QStringList terms = splitQuery(query);
    if (terms.isEmpty() || terms.first().isEmpty())
        return {};

    QSettings s("CachyOS", "UnifiedSearch");
    QString output = runProcess("pacman", QStringList{"-Ss"} + terms,
        s.value("pacmanTimeout", 15000).toInt());
    if (output.isEmpty()) return {};
    auto results = filterByRelevance(parsePacmanOutput(output), terms.first());
    for (auto &item : results)
        item.setIsInstalled(isPackageInstalled(item.name()));
    return results;
}

QList<SearchItem> SearchEngine::searchAur(const QString &query)
{
    if (m_aurHelper.isEmpty())
        return {};

    QStringList terms = splitQuery(query);
    if (terms.isEmpty() || terms.first().isEmpty())
        return {};

    QSettings s("CachyOS", "UnifiedSearch");
    QString output = runProcess(m_aurHelper, QStringList{"-Ssa"} + terms,
        s.value("aurTimeout", 30000).toInt());
    if (output.isEmpty()) return {};
    auto results = filterByRelevance(parseAurOutput(output), terms.first());
    for (auto &item : results)
        item.setIsInstalled(isPackageInstalled(item.name()));
    return results;
}

QList<SearchItem> SearchEngine::searchFlatpak(const QString &query)
{
    if (!m_hasFlatpak)
        return {};

    QStringList terms = splitQuery(query);
    if (terms.isEmpty() || terms.first().isEmpty())
        return {};

    refreshInstalledFlatpaks();

    QSettings s("CachyOS", "UnifiedSearch");
    QString output = runProcess("flatpak", QStringList{"search"} + terms,
        s.value("flatpakTimeout", 15000).toInt());
    if (output.isEmpty()) return {};
    auto results = filterByRelevance(parseFlatpakOutput(output), terms.first());
    for (auto &item : results)
        item.setIsInstalled(m_installedFlatpaks.contains(item.name()));
    return results;
}

SearchItem SearchEngine::parseSiOutput(const QString &output, SearchItem::Repository type)
{
    SearchItem item;
    item.setRepositoryType(type);

    for (const auto &line : output.split('\n')) {
        if (line.startsWith("Name")) {
            item.setName(line.section(':', 1).trimmed());
        } else if (line.startsWith("Version")) {
            item.setVersion(line.section(':', 1).trimmed());
        } else if (line.startsWith("Repository") || line.startsWith("Repository ")) {
            item.setRepository(line.section(':', 1).trimmed());
        } else if (line.startsWith("Description")) {
            item.setDescription(line.section(':', 1).trimmed());
        }
    }

    if (!item.name().isEmpty()) {
        item.setRelevanceScore(100);
        QString prefix;
        switch (type) {
        case SearchItem::Pacman:
            prefix = QString("[Repo: %1]").arg(item.repository());
            item.setInstallCommand(pacmanInstallCmd(item.name()));
            break;
        case SearchItem::AUR:
            prefix = "[AUR]";
            item.setInstallCommand(aurInstallCmd(m_aurHelper, item.name()));
            break;
        default:
            prefix = "[?]";
        }
        item.setDisplayName(QString("%1 %2 (%3)").arg(prefix, item.name(), item.version()));
    }
    return item;
}

int SearchEngine::computeRelevance(const QString &name, const QString &description,
                                    const QString &query)
{
    if (query.isEmpty()) return 1;

    QString lowerName = name.toLower();
    QString lowerDesc = description.toLower();
    QString lowerQuery = query.toLower();

    QStringList terms = lowerQuery.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
    if (terms.isEmpty()) return 1;

    int totalScore = 0;
    for (const auto &term : terms) {
        int termScore = 0;

        if (lowerName == term)
            termScore += 100;
        else if (lowerName.startsWith(term + '-') || lowerName.startsWith(term + '_'))
            termScore += 85;
        else if (lowerName.startsWith(term))
            termScore += 70;

        QRegularExpression wordBoundary("(^|[^a-zA-Z0-9])" + QRegularExpression::escape(term) + "($|[^a-zA-Z0-9])");
        if (wordBoundary.match(lowerName).hasMatch())
            termScore = qMax(termScore, 65);
        else if (lowerName.contains(term))
            termScore = qMax(termScore, 40);

        if (wordBoundary.match(lowerDesc).hasMatch())
            termScore = qMax(termScore, 25);
        else if (lowerDesc.contains(term))
            termScore = qMax(termScore, 5);

        totalScore += termScore;
    }

    return totalScore / terms.size();
}

QList<SearchItem> SearchEngine::filterByRelevance(QList<SearchItem> items,
                                                    const QString &query,
                                                    int minScore)
{
    for (auto &item : items)
        item.setRelevanceScore(computeRelevance(item.name(), item.description(), query));

    items.erase(
        std::remove_if(items.begin(), items.end(),
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

static QString ensureAskpassScript()
{
    static QString path;
    if (!path.isEmpty())
        return path;

    QString tmpPath = QStringLiteral("/tmp/cachy-askpass.sh");
    if (QFile::exists(tmpPath)) {
        path = tmpPath;
        return path;
    }

    QFile file(tmpPath);
    if (!file.open(QIODevice::WriteOnly))
        return {};

    QTextStream out(&file);
    out << "#!/bin/sh\n";
    out << "if command -v kdialog >/dev/null 2>&1; then\n";
    out << "    exec kdialog --title \"CachySearch\" --password \"$*\"\n";
    out << "elif command -v zenity >/dev/null 2>&1; then\n";
    out << "    exec zenity --title \"CachySearch\" --password\n";
    out << "else\n";
    out << "    exit 1\n";
    out << "fi\n";
    file.close();

    file.setPermissions(QFile::ExeOwner | QFile::ReadOwner
                        | QFile::ExeGroup | QFile::ReadGroup
                        | QFile::ExeOther | QFile::ReadOther);
    path = tmpPath;
    return path;
}

static bool hasGuiAskpass()
{
    if (!qEnvironmentVariableIsSet("DISPLAY")
        && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
        return false;
    return QFile::exists(QStringLiteral("/usr/bin/kdialog"))
        || QFile::exists(QStringLiteral("/usr/bin/zenity"));
}

QString SearchEngine::pacmanInstallCmd(const QString &package)
{
    return QString("pkexec pacman -Sy --noconfirm %1").arg(package);
}

QString SearchEngine::aurInstallCmd(const QString &helper, const QString &package)
{
    if (hasGuiAskpass()) {
        QString askpass = ensureAskpassScript();
        if (!askpass.isEmpty()) {
            // sudo -A: GUI password dialog (OS-handled, seguro)
            // SUDO_ASKPASS: script wrapper que usa kdialog (KDE) ou zenity
            // Cache do sudo por 5min: 1 dialogo apenas
            return QString("env SUDO_ASKPASS=%1 %2 --sudoflags=-A -S --noconfirm %3")
                .arg(askpass, helper, package);
        }
    }
    // Fallback: leitura da senha pelo terminal (stdin)
    return QString("%1 --sudoflags=-S -S --noconfirm %2").arg(helper, package);
}

QString SearchEngine::flatpakInstallCmd(const QString &remote, const QString &appId)
{
    return QString("flatpak install -y %1 %2").arg(remote, appId);
}

QString SearchEngine::uninstallCmd(const QString &packageName, SearchItem::Repository type)
{
    switch (type) {
    case SearchItem::Pacman:
    case SearchItem::AUR:
        return QString("pkexec pacman -Rns --noconfirm %1").arg(packageName);
    case SearchItem::Flatpak:
        return QString("flatpak uninstall -y %1").arg(packageName);
    }
    return {};
}

QList<SearchItem> SearchEngine::checkPacmanUpdates()
{
    QList<SearchItem> results;
    QString output = runProcess("checkupdates", {}, 60000);
    if (output.isEmpty()) return results;

    const auto lines = output.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 4) continue;
        if (parts[2] != "->") continue;
        if (parts[1] == parts[3]) continue;

        QString pkgName = parts[0];
        QString oldVer  = parts[1];
        QString newVer  = parts[3];

        SearchItem item;
        item.setName(pkgName);
        item.setVersion(QString("%1 → %2").arg(oldVer, newVer));
        item.setRepository("pacman");
        item.setRepositoryType(SearchItem::Pacman);
        item.setDisplayName(QString("[Repo] %1 (%2 → %3)").arg(pkgName, oldVer, newVer));
        item.setInstallCommand(pacmanInstallCmd(pkgName));
        item.setIsInstalled(true);
        results.append(item);
    }
    return results;
}

QList<SearchItem> SearchEngine::checkAurUpdates()
{
    QList<SearchItem> results;
    if (m_aurHelper.isEmpty()) return results;

    QString output = runProcess(m_aurHelper, {"-Qua"}, 30000);
    if (output.isEmpty()) return results;

    const auto lines = output.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 4 || parts[2] != "->") continue;
        if (parts[1] == parts[3]) continue;

        SearchItem item;
        item.setName(parts[0]);
        item.setVersion(QString("%1 → %2").arg(parts[1], parts[3]));
        item.setRepository("aur");
        item.setRepositoryType(SearchItem::AUR);
        item.setDisplayName(QString("[AUR] %1 (%2 → %3)").arg(parts[0], parts[1], parts[3]));
        item.setInstallCommand(aurInstallCmd(m_aurHelper, parts[0]));
        item.setIsInstalled(true);
        results.append(item);
    }
    return results;
}

QList<SearchItem> SearchEngine::checkFlatpakUpdates()
{
    QList<SearchItem> results;
    if (!m_hasFlatpak) return results;

    QString output = runProcess("flatpak", {"remote-ls", "--updates"}, 30000);
    if (output.isEmpty()) return results;

    const auto lines = output.split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        QStringList parts = trimmed.split('\t', Qt::KeepEmptyParts);
        if (parts.size() < 5) continue;

        QString appId = parts[1];
        if (!appId.contains('.')) continue;

        QString version = parts[3];
        SearchItem item;
        item.setName(appId);
        item.setVersion(version);
        item.setRepository(parts[4]);
        item.setRepositoryType(SearchItem::Flatpak);
        item.setDisplayName(QString("[Flatpak] %1 (%2)").arg(appId, version));
        item.setInstallCommand(QString("flatpak update -y %1").arg(appId));
        item.setIsInstalled(true);
        results.append(item);
    }
    return results;
}

SearchItem SearchEngine::queryPacmanPackageInfo(const QString &name)
{
    QString info = runProcess("pacman", {"-Si", name}, 3000);
    if (info.isEmpty()) return {};
    SearchItem si = parseSiOutput(info, SearchItem::Pacman);
    si.setIsInstalled(isPackageInstalled(name));
    return si;
}

SearchItem SearchEngine::queryAurPackageInfo(const QString &name)
{
    if (m_aurHelper.isEmpty()) return {};
    QString info = runProcess(m_aurHelper, {"-Si", name}, 5000);
    if (info.isEmpty()) return {};
    SearchItem si = parseSiOutput(info, SearchItem::AUR);
    si.setIsInstalled(isPackageInstalled(name));
    return si;
}
