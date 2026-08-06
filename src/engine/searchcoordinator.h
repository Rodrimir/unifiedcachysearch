#ifndef SEARCHCOORDINATOR_H
#define SEARCHCOORDINATOR_H

#include <QFutureWatcher>
#include <QList>
#include <QObject>
#include <QStringListModel>
#include "core/command.h"
#include "core/searchitem.h"
#include "core/searchmodel.h"
#include "engine/commandrunner.h"
#include "engine/searchengine.h"

// Orquestra as tres buscas paralelas, o cross-reference e a fila de comandos.
// Nada aqui bloqueia a thread da UI: cada etapa e uma tarefa do QtConcurrent
// com o resultado descartado se a busca ja tiver sido substituida.
class SearchCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit SearchCoordinator(SearchModel *model, QObject *parent = nullptr);
    ~SearchCoordinator() override;

    void search(const QString &query);
    void cancelSearch();
    bool isSearching() const { return m_searchesPending > 0; }
    SearchEngine *engine() const { return m_engine; }
    QString aurHelper() const { return m_engine->aurHelper(); }

    void refreshInstalledStatus();

    void saveSearchHistory(const QString &query);
    void loadSearchHistory();
    QStringListModel *historyModel() const { return m_historyModel; }

    void setRunner(CommandRunner *runner);
    void runCommand(const Command &command, const QString &actionLabel);
    void runCommandsSequentially(const QList<Command> &commands, const QString &overallLabel);
    QList<Command> buildInstallCommands(const QList<SearchItem> &items) const;

    SearchItem itemAt(int row) const { return m_model->itemAt(row); }
    QList<SearchItem> allItems() const { return m_model->allItems(); }
    int resultCount() const { return m_model->rowCount(); }
    Command installCommandAt(int row) const { return m_model->itemAt(row).installCommand(); }
    Command uninstallCommandFor(const SearchItem &item) const
    {
        return m_engine->commands().uninstall(item.name(), item.repositoryType());
    }

signals:
    void progressChanged(const QString &text);
    void progressBarVisible(bool visible);
    void commandFinished(bool success, const QString &action);
    void allCommandsFinished(const QString &overallLabel);
    void processAttached(QProcess *process);
    void consoleMessage(const QString &text, int type);
    void resultsAdded(int count, int pending, int total);
    void crossReferenceStarted();
    void searchFinished(int total);
    void statusChanged(const QString &text);

private:
    using ResultWatcher = QFutureWatcher<QList<SearchItem>>;

    void connectWatcher(ResultWatcher *watcher, int generation, QList<SearchItem> *target);
    void startCrossReference(int generation);
    CrossReferenceResult crossReferenceResults(const QList<SearchItem> &pacman,
                                               const QList<SearchItem> &aur,
                                               const QList<SearchItem> &flatpak);
    void applyCrossRef(const CrossReferenceResult &result);
    void onCrossRefFinished();
    void applyInstalledStatus();
    void waitForTasks();

    SearchEngine *m_engine;
    SearchModel *m_model;
    CommandRunner *m_runner = nullptr;
    QStringListModel *m_historyModel;

    ResultWatcher *m_pacmanWatcher;
    ResultWatcher *m_aurWatcher;
    ResultWatcher *m_flatpakWatcher;
    QFutureWatcher<CrossReferenceResult> *m_crossRefWatcher;
    QFutureWatcher<void> *m_installedWatcher;

    QList<SearchItem> m_pacmanResults;
    QList<SearchItem> m_aurResults;
    QList<SearchItem> m_flatpakResults;

    int m_searchesPending = 0;
    int m_totalResults = 0;
    int m_searchGeneration = 0;
    int m_crossRefGeneration = -1;
};

#endif
