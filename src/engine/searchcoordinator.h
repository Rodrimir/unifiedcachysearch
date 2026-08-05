#ifndef SEARCHCOORDINATOR_H
#define SEARCHCOORDINATOR_H

#include <QObject>
#include <QList>
#include <QFutureWatcher>
#include <QStringListModel>
#include "core/searchitem.h"
#include "engine/searchengine.h"
#include "core/searchmodel.h"
#include "engine/commandrunner.h"

class SearchCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit SearchCoordinator(SearchModel *model, QObject *parent = nullptr);

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
    void runCommand(const QString &command, const QString &actionLabel);
    void runCommandsSequentially(const QStringList &commands, const QString &overallLabel);
    QStringList buildInstallCommands(const QList<SearchItem> &items) const;

    SearchItem itemAt(int row) const { return m_model->itemAt(row); }
    QList<SearchItem> allItems() const { return m_model->allItems(); }
    int resultCount() const { return m_model->rowCount(); }
    QString installCommandAt(int row) const { return m_model->itemAt(row).installCommand(); }

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
    void connectWatcher(QFutureWatcher<QList<SearchItem>> *watcher,
                        const QString &label, int gen);
    CrossReferenceResult crossReferenceResults(
        const QList<SearchItem> &pacman,
        const QList<SearchItem> &aur,
        const QList<SearchItem> &flatpak);
    void applyCrossRef(const QList<SearchItem> &pacman,
                       const QList<SearchItem> &aur,
                       const QList<SearchItem> &flatpak);
    void onCrossRefFinished();

    SearchEngine *m_engine;
    SearchModel *m_model;
    CommandRunner *m_runner = nullptr;
    QStringListModel *m_historyModel;

    QFutureWatcher<QList<SearchItem>> *m_pacmanWatcher;
    QFutureWatcher<QList<SearchItem>> *m_aurWatcher;
    QFutureWatcher<QList<SearchItem>> *m_flatpakWatcher;
    QFutureWatcher<CrossReferenceResult> *m_crossRefWatcher;

    int m_searchesPending;
    int m_totalResults;
    int m_searchGeneration;
};

#endif
