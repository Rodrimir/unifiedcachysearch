#include "engine/searchcoordinator.h"
#include "core/appsettings.h"
#include <QMap>
#include <QSet>
#include <QThread>
#include <QtConcurrent>

namespace {
// Pausa entre consultas no modo "Mínimo (menos CPU)" do cross-reference.
constexpr int CrossRefThrottleMs = 10;
} // namespace

SearchCoordinator::SearchCoordinator(SearchModel *model, QObject *parent)
    : QObject(parent)
    , m_engine(new SearchEngine(this))
    , m_model(model)
    , m_historyModel(new QStringListModel(this))
    , m_pacmanWatcher(new ResultWatcher(this))
    , m_aurWatcher(new ResultWatcher(this))
    , m_flatpakWatcher(new ResultWatcher(this))
    , m_crossRefWatcher(new QFutureWatcher<CrossReferenceResult>(this))
    , m_installedWatcher(new QFutureWatcher<void>(this))
{
    connect(m_crossRefWatcher, &QFutureWatcher<CrossReferenceResult>::finished,
            this, &SearchCoordinator::onCrossRefFinished);
    connect(m_installedWatcher, &QFutureWatcher<void>::finished,
            this, &SearchCoordinator::applyInstalledStatus);
}

SearchCoordinator::~SearchCoordinator()
{
    // As tarefas usam o engine, que e filho deste objeto: precisamos garantir
    // que nenhuma continue viva quando ele for destruido.
    m_engine->cancelSearch();
    waitForTasks();
}

void SearchCoordinator::waitForTasks()
{
    for (ResultWatcher *watcher : {m_pacmanWatcher, m_aurWatcher, m_flatpakWatcher})
        watcher->waitForFinished();
    m_crossRefWatcher->waitForFinished();
    m_installedWatcher->waitForFinished();
}

// --- Busca ----------------------------------------------------------------

void SearchCoordinator::search(const QString &query)
{
    cancelSearch();

    // O refresh em si acontece dentro das tarefas, fora da thread da UI.
    m_engine->invalidateInstalledCache();
    m_pacmanResults.clear();
    m_aurResults.clear();
    m_flatpakResults.clear();
    m_totalResults = 0;

    emit statusChanged(QString("Pesquisando \"%1\"...").arg(query));

    const int generation = ++m_searchGeneration;
    m_searchesPending = m_engine->hasFlatpak() ? 3 : 2;

    connectWatcher(m_pacmanWatcher, generation, &m_pacmanResults);
    m_pacmanWatcher->setFuture(QtConcurrent::run([this, query]() {
        return m_engine->searchPacman(query);
    }));

    connectWatcher(m_aurWatcher, generation, &m_aurResults);
    m_aurWatcher->setFuture(QtConcurrent::run([this, query]() {
        return m_engine->searchAur(query);
    }));

    if (m_engine->hasFlatpak()) {
        connectWatcher(m_flatpakWatcher, generation, &m_flatpakResults);
        m_flatpakWatcher->setFuture(QtConcurrent::run([this, query]() {
            return m_engine->searchFlatpak(query);
        }));
    }
}

void SearchCoordinator::cancelSearch()
{
    // Trocar a geracao ja faz os resultados pendentes serem ignorados; nao
    // esperamos as tarefas aqui porque isso congelava a UI por segundos.
    ++m_searchGeneration;
    m_searchesPending = 0;
    m_engine->cancelSearch();

    m_pacmanWatcher->future().cancel();
    m_aurWatcher->future().cancel();
    m_flatpakWatcher->future().cancel();
    m_crossRefWatcher->future().cancel();

    m_model->clear();
    m_totalResults = 0;
}

void SearchCoordinator::connectWatcher(ResultWatcher *watcher, int generation,
                                       QList<SearchItem> *target)
{
    disconnect(watcher, &ResultWatcher::finished, this, nullptr);
    connect(watcher, &ResultWatcher::finished, this, [this, watcher, generation, target]() {
        // Resultado de uma busca ja substituida, ou future cancelada (cujo
        // result() nao pode ser lido).
        if (generation != m_searchGeneration || watcher->isCanceled())
            return;

        const QList<SearchItem> results = watcher->result();
        *target = results;
        m_model->addItems(results);
        m_totalResults += results.size();

        if (--m_searchesPending > 0) {
            emit resultsAdded(results.size(), m_searchesPending, m_totalResults);
            return;
        }
        startCrossReference(generation);
    });
}

void SearchCoordinator::startCrossReference(int generation)
{
    emit statusChanged("Fazendo cross-reference entre Pacman e AUR...");
    emit crossReferenceStarted();

    const QList<SearchItem> pacman = m_pacmanResults;
    const QList<SearchItem> aur = m_aurResults;
    const QList<SearchItem> flatpak = m_flatpakResults;

    m_crossRefGeneration = generation;
    m_crossRefWatcher->setFuture(QtConcurrent::run([this, pacman, aur, flatpak]() {
        return crossReferenceResults(pacman, aur, flatpak);
    }));
}

void SearchCoordinator::onCrossRefFinished()
{
    if (m_crossRefWatcher->isCanceled() || m_crossRefGeneration != m_searchGeneration)
        return;

    const CrossReferenceResult result = m_crossRefWatcher->result();
    m_model->clear();
    m_totalResults = 0;
    applyCrossRef(result);
    emit searchFinished(m_totalResults);
}

CrossReferenceResult SearchCoordinator::crossReferenceResults(
    const QList<SearchItem> &pacmanInput,
    const QList<SearchItem> &aurInput,
    const QList<SearchItem> &flatpakInput)
{
    const int speed = AppSettings::crossRefSpeed();
    CrossReferenceResult result{pacmanInput, aurInput, flatpakInput};
    if (speed == 0)
        return result;

    QSet<QString> pacmanNames, aurNames;
    for (const auto &item : result.pacman) pacmanNames.insert(item.name());
    for (const auto &item : result.aur) aurNames.insert(item.name());

    // Pacotes que so o AUR trouxe podem existir tambem nos repos oficiais.
    QList<SearchItem> pacmanCandidates;
    for (const auto &item : result.aur)
        if (!pacmanNames.contains(item.name()))
            pacmanCandidates.append(item);

    auto absorb = [](QList<SearchItem> &into, QSet<QString> &known, const SearchItem &item) {
        if (!item.name().isEmpty() && !known.contains(item.name())) {
            into.append(item);
            known.insert(item.name());
        }
    };

    if (!pacmanCandidates.isEmpty()) {
        if (speed >= 2) {
            QFuture<SearchItem> future = QtConcurrent::mapped(pacmanCandidates,
                [this](const SearchItem &item) {
                    return m_engine->queryPacmanPackageInfo(item.name());
                });
            future.waitForFinished();
            for (const auto &info : future.results())
                absorb(result.pacman, pacmanNames, info);
        } else {
            for (const auto &item : pacmanCandidates) {
                QThread::msleep(CrossRefThrottleMs);
                absorb(result.pacman, pacmanNames, m_engine->queryPacmanPackageInfo(item.name()));
            }
        }
    }

    if (!m_engine->aurHelper().isEmpty()) {
        QList<SearchItem> aurCandidates;
        for (const auto &item : result.pacman)
            if (!aurNames.contains(item.name()))
                aurCandidates.append(item);

        if (!aurCandidates.isEmpty()) {
            if (speed >= 2) {
                QFuture<SearchItem> future = QtConcurrent::mapped(aurCandidates,
                    [this](const SearchItem &item) {
                        return m_engine->queryAurPackageInfo(item.name());
                    });
                future.waitForFinished();
                for (const auto &info : future.results())
                    absorb(result.aur, aurNames, info);
            } else {
                for (const auto &item : aurCandidates) {
                    QThread::msleep(CrossRefThrottleMs);
                    absorb(result.aur, aurNames, m_engine->queryAurPackageInfo(item.name()));
                }
            }
        }
    }

    return result;
}

void SearchCoordinator::applyCrossRef(const CrossReferenceResult &result)
{
    auto addAll = [this](const QList<SearchItem> &list) {
        m_totalResults += list.size();
        m_model->addItems(list);
    };
    addAll(result.pacman);
    addAll(result.aur);
    if (m_engine->hasFlatpak())
        addAll(result.flatpak);
}

// --- Estado de instalacao -------------------------------------------------

void SearchCoordinator::refreshInstalledStatus()
{
    if (m_installedWatcher->isRunning())
        return;

    // Ler o estado instalado dispara pacman -Qq e flatpak list: fora da UI.
    m_installedWatcher->setFuture(QtConcurrent::run([this]() {
        m_engine->refreshInstalledPackages();
        m_engine->refreshInstalledFlatpaks();
    }));
}

void SearchCoordinator::applyInstalledStatus()
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        SearchItem item = m_model->itemAt(row);
        const bool installed = item.repositoryType() == SearchItem::Flatpak
            ? m_engine->isFlatpakInstalled(item.name())
            : m_engine->isPackageInstalled(item.name());
        if (item.isInstalled() != installed) {
            item.setIsInstalled(installed);
            m_model->setItemAt(row, item);
        }
    }
}

// --- Comandos -------------------------------------------------------------

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

void SearchCoordinator::runCommand(const Command &command, const QString &actionLabel)
{
    if (m_runner) m_runner->runCommand(command, actionLabel);
}

void SearchCoordinator::runCommandsSequentially(const QList<Command> &commands,
                                                const QString &overallLabel)
{
    if (m_runner) m_runner->runCommandsSequentially(commands, overallLabel);
}

QList<Command> SearchCoordinator::buildInstallCommands(const QList<SearchItem> &items) const
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

    const CommandBuilder &builder = m_engine->commands();
    QList<Command> commands;
    if (!pacmanPkgs.isEmpty())
        commands << builder.installPacman(pacmanPkgs);
    if (!aurPkgs.isEmpty() && builder.hasAurHelper())
        commands << builder.installAur(aurPkgs);
    for (auto it = flatpakByRemote.cbegin(); it != flatpakByRemote.cend(); ++it)
        commands << builder.installFlatpak(it.key(), it.value());

    return commands;
}

// --- Historico ------------------------------------------------------------

void SearchCoordinator::saveSearchHistory(const QString &query)
{
    QStringList history = m_historyModel->stringList();
    history.removeAll(query);
    history.prepend(query);

    const int limit = AppSettings::historyLimit();
    if (history.size() > limit)
        history = history.mid(0, limit);

    m_historyModel->setStringList(history);
    AppSettings::setSearchHistory(history);
}

void SearchCoordinator::loadSearchHistory()
{
    m_historyModel->setStringList(AppSettings::searchHistory());
}
