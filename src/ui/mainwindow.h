#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QListView>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QCompleter>
#include <QEvent>
#include <QStackedWidget>
#include <QTabBar>
#include <QFutureWatcher>
#include "core/command.h"
#include "core/searchmodel.h"
#include "core/sortfilterproxymodel.h"
#include "ui/packagedelegate.h"
#include "ui/detailpanel.h"
#include "ui/outputconsole.h"
#include "engine/searchcoordinator.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSearchClicked();
    void onInstallClicked(const Command &command);
    void onUninstallClicked(const Command &command);
    void onInstallAllClicked();
    void onClearClicked();
    void onSearchTextChanged(const QString &text);
    void onSelectionChanged();
    void onThemeToggle();
    void onExportClicked();
    void onSettingsClicked();
    void onCheckUpdatesClicked();

    void onCoordinatorStatus(const QString &text);
    void onCoordinatorResultsAdded(int count, int pending, int total);
    void onCrossReferenceStarted();
    void onSearchFinished(int total);

    void onCommandProgress(const QString &text);
    void onCommandProgressBar(bool visible);
    void onCommandFinished(bool success, const QString &action);
    void onAllCommandsFinished(const QString &overallLabel);

private:
    void setupTitleBar(QVBoxLayout *mainLayout);
    void setupModeBar(QVBoxLayout *mainLayout);
    void setupSharedFilters(QVBoxLayout *mainLayout);
    void setupContent(QVBoxLayout *mainLayout);
    void setupStatusBar(QVBoxLayout *mainLayout);
    void applyStyles(const QString &theme = "dark");
    void setupConnections();
    void exportResults(const QString &filePath);
    void switchToSearchMode();
    void switchToUpdateMode();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QWidget *m_titleBar;
    QLabel *m_titleLabel;
    QPushButton *m_themeButton;
    QPushButton *m_soundButton;
    QPushButton *m_settingsButton;
    QPushButton *m_minButton;
    QPushButton *m_maxButton;
    QPushButton *m_closeButton;
    QTabBar *m_modeTab;

    QStackedWidget *m_modeStack;
    // Page 0 - search
    QLineEdit *m_searchInput;
    QPushButton *m_searchButton;
    // Page 1 - updates
    QPushButton *m_checkUpdatesBtn;
    QLabel *m_updateInfoLabel;
    QPushButton *m_updateSelectedBtn;

    QPushButton *m_installAllButton;
    QPushButton *m_clearButton;
    QPushButton *m_exportButton;

    QPushButton *m_filterPacman;
    QPushButton *m_filterAur;
    QPushButton *m_filterFlatpak;

    QWidget *m_filterWidget;
    QListView *m_resultView;
    DetailPanel *m_detailPanel;
    OutputConsole *m_console;
    QLabel *m_countLabel;
    QLabel *m_progressLabel;
    QProgressBar *m_progressBar;

    PackageDelegate *m_delegate;
    SearchModel *m_model;
    SortFilterProxyModel *m_proxyModel;
    SearchCoordinator *m_coordinator;
    QTimer *m_searchTimer;
    QCompleter *m_completer;
    QFutureWatcher<QList<SearchItem>> *m_updatesWatcher;

    bool m_isDark;
    bool m_inUpdateMode = false;
    int m_totalResults = 0;
    QPoint m_dragPosition;
    bool m_dragging;
    int m_resizeEdge;
    QPoint m_resizeStartPos;
    QRect m_resizeStartGeo;
};

#endif
