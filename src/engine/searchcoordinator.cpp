#include "engine/searchcoordinator.h"
#include <QtConcurrent>
#include <QSet>
#include <QSettings>
#include <QThread>

SearchCoordinator::SearchCoordinator(SearchModel *model, QObject *parent)
    : QObject(parent)
    , m_engine(new SearchEngine(this))
    , m_model(model)
    , m_historyModel(new QStringListModel(this))
    , m_pacmanWatcher(new QFutureWatcher<QList<SearchItem>>(this))
    , m_aurWatcher(new QFutureWatcher<QList<SearchItem>>(this))
    , m_flatpakWatcher(new QFutureWatcher<QList<SearchItem>>(this))
    , m_crossRefWatcher(new QFutureWatcher<CrossReferenceResult>(this))
    , m_searchesPending(0)
    , m_totalResults(0)
    , m_searchGeneration(0)
{
    connect(m_crossRefWatcher, &QFutureWatcher<CrossReferenceResult>::finished,
            this, &SearchCoordinator::onCrossRefFinished);
}

void SearchCoordinator::search(const QString &query)
{
    cancelSearch();
    m_engine->refreshInstalledPkgs();
    m_totalResults = 0;
    m_searchesPending = 0;

    emit statusChanged(QString("Pesquisando \"%1\"...").arg(query));

    int gen = ++m_searchGeneration;
    int active = 0;

    connectWatcher(m_pacmanWatcher, "Repositórios oficiais", gen);
    m_pacmanWatcher->setFuture(QtConcurrent::run([this, query]() {
        return m_engine->searchPacman(query);
    }));
    active++;

    connectWatcher(m_aurWatcher, "AUR", gen);
    m_aurWatcher->setFuture(QtConcurrent::run([this, query]() {
        return m_engine->searchAur(query);
    }));
    active++;

    if (m_engine->hasFlatpak()) {
        connectWatcher(m_flatpakWatcher, "Flatpak", gen);
        m_flatpakWatcher->setFuture(QtConcurrent::run([this, query]() {
            return m_engine->searchFlatpak(query);
        }));
        active++;
    }

    m_searchesPending = active;
}

void SearchCoordinator::cancelSearch()
{
    m_searchGeneration++;
    m_searchesPending = 0;
    m_engine->cancelSearch();

    if (m_crossRefWatcher->isRunning()) {
        m_crossRefWatcher->future().cancel();
        m_crossRefWatcher->waitForFinished();
    }

    m_pacmanWatcher->future().cancel();
    m_aurWatcher->future().cancel();
    m_flatpakWatcher->future().cancel();

    m_model->clear();
    m_totalResults = 0;
}

void SearchCoordinator::connectWatcher(
    QFutureWatcher<QList<SearchItem>> *watcher,
    const QString &label, int gen)
{
    disconnect(watcher, &QFutureWatcher<QList<SearchItem>>::finished, this, nullptr);
    connect(watcher, &QFutureWatcher<QList<SearchItem>>::finished, this,
            [this, gen, watcher, label]() {
        if (gen != m_searchGeneration) return;
        m_totalResults += watcher->result().size();
        m_model->addItems(watcher->result());
        m_searchesPending--;
        if (m_searchesPending > 0) {
            emit resultsAdded(watcher->result().size(), m_searchesPending, m_totalResults);
            return;
        }

        QList<SearchItem> pacmanList = m_pacmanWatcher->result();
        QList<SearchItem> aurList = m_aurWatcher->result();
        QList<SearchItem> flatpakList = m_engine->hasFlatpak()
            ? m_flatpakWatcher->result() : QList<SearchItem>();

        emit statusChanged("Fazendo cross-reference entre Pacman e AUR...");
        emit crossReferenceStarted();

        m_crossRefWatcher->setFuture(
            QtConcurrent::run([this, pacmanList, aurList, flatpakList]() {
                return crossReferenceResults(pacmanList, aurList, flatpakList);
            }));
    });
}

void SearchCoordinator::onCrossRefFinished()
{
    CrossReferenceResult result = m_crossRefWatcher->result();
    m_model->clear();
    m_totalResults = 0;
    applyCrossRef(result.pacman, result.aur, result.flatpak);
    emit searchFinished(m_totalResults);
}

CrossReferenceResult SearchCoordinator::crossReferenceResults(
    const QList<SearchItem> &pacmanInput,
    const QList<SearchItem> &aurInput,
    const QList<SearchItem> &flatpakInput)
{
    QSettings s("CachyOS", "UnifiedSearch");
    int speed = s.value("crossRefSpeed", 1).toInt();

    if (speed == 0)
        return {pacmanInput, aurInput, flatpakInput};

    CrossReferenceResult result{pacmanInput, aurInput, flatpakInput};
    QSet<QString> pacmanNames, aurNames;
    for (const auto &i : result.pacman) pacmanNames.insert(i.name());
    for (const auto &i : result.aur) aurNames.insert(i.name());

    QList<SearchItem> pacmanCandidates;
    for (const auto &i : result.aur)
        if (!pacmanNames.contains(i.name()))
            pacmanCandidates.append(i);

    if (!pacmanCandidates.isEmpty()) {
        if (speed >= 2) {
            QFuture<SearchItem> future = QtConcurrent::mapped(pacmanCandidates,
                [this](const SearchItem &item) {
                    return m_engine->queryPacmanPackageInfo(item.name());
                });
            future.waitForFinished();
            for (const auto &si : future.results()) {
                if (!si.name().isEmpty() && !pacmanNames.contains(si.name())) {
                    result.pacman.append(si);
                    pacmanNames.insert(si.name());
                }
            }
        } else {
            for (const auto &item : pacmanCandidates) {
                QThread::msleep(10);
                SearchItem si = m_engine->queryPacmanPackageInfo(item.name());
                if (!si.name().isEmpty() && !pacmanNames.contains(si.name())) {
                    result.pacman.append(si);
                    pacmanNames.insert(si.name());
                }
            }
        }
    }

    if (!m_engine->aurHelper().isEmpty()) {
        QList<SearchItem> aurCandidates;
        for (const auto &i : result.pacman)
            if (!aurNames.contains(i.name()))
                aurCandidates.append(i);

        if (!aurCandidates.isEmpty()) {
            if (speed >= 2) {
                QFuture<SearchItem> future = QtConcurrent::mapped(aurCandidates,
                    [this](const SearchItem &item) {
                        return m_engine->queryAurPackageInfo(item.name());
                    });
                future.waitForFinished();
                for (const auto &si : future.results()) {
                    if (!si.name().isEmpty() && !aurNames.contains(si.name())) {
                        result.aur.append(si);
                        aurNames.insert(si.name());
                    }
                }
            } else {
                for (const auto &item : aurCandidates) {
                    QThread::msleep(10);
                    SearchItem si = m_engine->queryAurPackageInfo(item.name());
                    if (!si.name().isEmpty() && !aurNames.contains(si.name())) {
                        result.aur.append(si);
                        aurNames.insert(si.name());
                    }
                }
            }
        }
    }

    return result;
}

void SearchCoordinator::applyCrossRef(const QList<SearchItem> &pacman,
                                       const QList<SearchItem> &aur,
                                       const QList<SearchItem> &flatpak)
{
    auto addAll = [&](const QList<SearchItem> &list) {
        m_totalResults += list.size();
        m_model->addItems(list);
    };
    addAll(pacman);
    addAll(aur);
    if (m_engine->hasFlatpak())
        addAll(flatpak);
}

void SearchCoordinator::setRunner(CommandRunner *runner)
{
    m_runner = runner;
    connect(m_runner, &CommandRunner::progressChanged, this, &SearchCoordinator::progressChanged);
    connect(m_runner, &CommandRunner::progressBarVisible, this, &SearchCoordinator::progressBarVisible);
    connect(m_runner, &CommandRunner::commandFinished, this, &SearchCoordinator::commandFinished);
    connect(m_runner, &CommandRunner::allCommandsFinished, this, &SearchCoordinator::allCommandsFinished);
    connect(m_runner, &CommandRunner::processAttached, this, &SearchCoordinator::processAttached);
    connect(m_runner, &CommandRunner::consoleMessage, this, &SearchCoordinator::consoleMessage);
}

void SearchCoordinator::runCommand(const QString &command, const QString &actionLabel)
{
    if (m_runner) m_runner->runCommand(command, actionLabel);
}

void SearchCoordinator::runCommandsSequentially(const QStringList &commands, const QString &overallLabel)
{
    if (m_runner) m_runner->runCommandsSequentially(commands, overallLabel);
}

QStringList SearchCoordinator::buildInstallCommands(const QList<SearchItem> &items) const
{
    QStringList pacmanPkgs, aurPkgs;
    QMap<QString, QStringList> flatpakByRemote;

    for (const auto &item : items) {
        switch (item.repositoryType()) {
        case SearchItem::Pacman:
            pacmanPkgs << item.name();
            break;
        case SearchItem::AUR:
            aurPkgs << item.name();
            break;
        case SearchItem::Flatpak:
            flatpakByRemote[item.repository()] << item.name();
            break;
        }
    }

    QStringList cmds;
    if (!pacmanPkgs.isEmpty())
        cmds << SearchEngine::pacmanInstallCmd(pacmanPkgs.join(' '));
    if (!aurPkgs.isEmpty()) {
        QString helper = m_engine->aurHelper();
        if (!helper.isEmpty())
            cmds << SearchEngine::aurInstallCmd(helper, aurPkgs.join(' '));
    }
    for (auto it = flatpakByRemote.cbegin(); it != flatpakByRemote.cend(); ++it)
        cmds << SearchEngine::flatpakInstallCmd(it.key(), it.value().join(' '));

    return cmds;
}

void SearchCoordinator::refreshInstalledStatus()
{
    m_engine->refreshInstalledPkgs();
    m_engine->refreshInstalledFlatpaks();
    for (int i = 0; i < m_model->rowCount(); ++i) {
        SearchItem item = m_model->itemAt(i);
        bool installed = item.repositoryType() == SearchItem::Flatpak
            ? m_engine->isFlatpakInstalled(item.name())
            : m_engine->isPackageInstalled(item.name());
        if (item.isInstalled() != installed) {
            item.setIsInstalled(installed);
            m_model->setItemAt(i, item);
        }
    }
}

void SearchCoordinator::saveSearchHistory(const QString &query)
{
    QStringList history = m_historyModel->stringList();
    history.removeAll(query);
    history.prepend(query);
    QSettings s("CachyOS", "UnifiedSearch");
    int limit = s.value("historyLimit", 20).toInt();
    if (history.size() > limit)
        history = history.mid(0, limit);
    m_historyModel->setStringList(history);
    s.setValue("searchHistory", history);
}

void SearchCoordinator::loadSearchHistory()
{
    QSettings settings("CachyOS", "UnifiedSearch");
    QStringList history = settings.value("searchHistory").toStringList();
    m_historyModel->setStringList(history);
}
