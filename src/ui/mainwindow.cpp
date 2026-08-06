#include "ui/mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QShortcut>
#include <QGraphicsDropShadowEffect>
#include <QStyleHints>
#include <QApplication>
#include <QFileDialog>
#include <QMouseEvent>
#include <QtConcurrent>
#include <QFutureWatcher>

#include "audio/soundmanager.h"
#include "core/appsettings.h"
#include "core/exportservice.h"
#include "ui/settingsdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_delegate(new PackageDelegate(this))
    , m_model(new SearchModel(this))
    , m_proxyModel(new SortFilterProxyModel(this))
    , m_coordinator(new SearchCoordinator(m_model, this))
    , m_searchTimer(new QTimer(this))
    , m_completer(new QCompleter(this))
    , m_updatesWatcher(new QFutureWatcher<QList<SearchItem>>(this))
    , m_dragging(false)
    , m_resizeEdge(0)
{
    const auto colorScheme = QApplication::styleHints()->colorScheme();
    const bool systemIsDark = (colorScheme == Qt::ColorScheme::Dark ||
                               colorScheme == Qt::ColorScheme::Unknown);
    m_isDark = AppSettings::resolveDarkTheme(AppSettings::theme(), systemIsDark);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setMouseTracking(true);
    QApplication::instance()->installEventFilter(this);

    m_proxyModel->setSourceModel(m_model);

    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupTitleBar(mainLayout);
    setupModeBar(mainLayout);
    setupSharedFilters(mainLayout);
    setupContent(mainLayout);
    setupStatusBar(mainLayout);

    setCentralWidget(centralWidget);
    applyStyles(m_isDark ? "dark" : "light");
    setupConnections();
    m_coordinator->loadSearchHistory();
    switchToSearchMode();

    setWindowTitle("CachyOS Unified Search");
    resize(960, 680);
    setMinimumSize(720, 500);
}

MainWindow::~MainWindow()
{
    // A verificacao de atualizacoes roda numa thread e usa o engine; esperar
    // aqui evita que ela sobreviva a janela.
    m_updatesWatcher->waitForFinished();
}

void MainWindow::setupTitleBar(QVBoxLayout *mainLayout)
{
    m_titleBar = new QWidget(this);
    m_titleBar->setFixedHeight(40);
    m_titleBar->setObjectName("titleBar");

    auto *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(12, 0, 4, 0);

    m_titleLabel = new QLabel("CachyOS Unified Search", this);
    m_titleLabel->setObjectName("titleLabel");

    m_modeTab = new QTabBar(this);
    m_modeTab->addTab("Pesquisa");
    m_modeTab->addTab("Atualizações");
    m_modeTab->setObjectName("modeTab");
    m_modeTab->setDrawBase(false);

    m_themeButton = new QPushButton(QString::fromUtf8("\xE2\x98\xBC"), this);
    m_themeButton->setFixedSize(36, 28);
    m_themeButton->setObjectName("titleButton");
    m_themeButton->setToolTip("Alternar tema");
    m_themeButton->setFocusPolicy(Qt::NoFocus);

    m_soundButton = new QPushButton(QString::fromUtf8("\xF0\x9F\x94\x8A"), this);
    m_soundButton->setFixedSize(36, 28);
    m_soundButton->setObjectName("titleButton");
    m_soundButton->setToolTip("Desativar som");
    m_soundButton->setFocusPolicy(Qt::NoFocus);

    m_settingsButton = new QPushButton(QString::fromUtf8("\xE2\x9A\x99"), this);
    m_settingsButton->setFixedSize(36, 28);
    m_settingsButton->setObjectName("titleButton");
    m_settingsButton->setToolTip("Configurações");
    m_settingsButton->setFocusPolicy(Qt::NoFocus);

    m_minButton = new QPushButton("\xe2\x80\x94", this);
    m_minButton->setFixedSize(36, 28);
    m_minButton->setObjectName("titleButton");
    m_minButton->setToolTip("Minimizar");
    m_minButton->setFocusPolicy(Qt::NoFocus);

    m_maxButton = new QPushButton("\xe2\x96\xa1", this);
    m_maxButton->setFixedSize(36, 28);
    m_maxButton->setObjectName("titleButton");
    m_maxButton->setToolTip("Maximizar");
    m_maxButton->setFocusPolicy(Qt::NoFocus);

    m_closeButton = new QPushButton(QString::fromUtf8("\xE2\x9C\x95"), this);
    m_closeButton->setFixedSize(36, 28);
    m_closeButton->setObjectName("closeButton");
    m_closeButton->setToolTip("Fechar");
    m_closeButton->setFocusPolicy(Qt::NoFocus);

    titleLayout->addWidget(m_titleLabel);
    titleLayout->addSpacing(16);
    titleLayout->addWidget(m_modeTab);
    titleLayout->addStretch();
    titleLayout->addWidget(m_themeButton);
    titleLayout->addWidget(m_soundButton);
    titleLayout->addWidget(m_settingsButton);
    titleLayout->addWidget(m_minButton);
    titleLayout->addWidget(m_maxButton);
    titleLayout->addWidget(m_closeButton);

    mainLayout->addWidget(m_titleBar);
}

void MainWindow::setupModeBar(QVBoxLayout *mainLayout)
{
    m_modeStack = new QStackedWidget(this);

    // Page 0 - search mode
    auto *searchPage = new QWidget;
    auto *searchLayout = new QVBoxLayout(searchPage);
    searchLayout->setContentsMargins(16, 10, 16, 4);
    searchLayout->setSpacing(8);

    auto *searchBtnRow = new QHBoxLayout;
    searchBtnRow->setSpacing(6);
    m_searchButton = new QPushButton("Pesquisar", this);
    m_searchButton->setObjectName("searchButton");
    m_installAllButton = new QPushButton("Instalar Todos", this);
    m_installAllButton->setObjectName("installAllButton");
    m_exportButton = new QPushButton("Exportar", this);
    m_exportButton->setObjectName("exportButton");
    m_clearButton = new QPushButton("Limpar", this);
    m_clearButton->setObjectName("clearButton");

    searchBtnRow->addWidget(m_searchButton);
    searchBtnRow->addWidget(m_installAllButton);
    searchBtnRow->addWidget(m_exportButton);
    searchBtnRow->addWidget(m_clearButton);
    searchBtnRow->addStretch();
    searchLayout->addLayout(searchBtnRow);

    m_searchInput = new QLineEdit;
    m_searchInput->setPlaceholderText("Digite o nome do pacote para pesquisar...");
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->setMinimumHeight(40);

    m_completer->setModel(m_coordinator->historyModel());
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setMaxVisibleItems(8);
    m_searchInput->setCompleter(m_completer);

    searchLayout->addWidget(m_searchInput);
    m_modeStack->addWidget(searchPage);

    // Page 1 - update mode
    auto *updatePage = new QWidget;
    auto *updateLayout = new QVBoxLayout(updatePage);
    updateLayout->setContentsMargins(16, 10, 16, 4);
    updateLayout->setSpacing(8);

    auto *updateBtnRow = new QHBoxLayout;
    updateBtnRow->setSpacing(6);
    m_checkUpdatesBtn = new QPushButton("Verificar Atualizações", this);
    m_checkUpdatesBtn->setObjectName("checkUpdatesButton");
    m_updateSelectedBtn = new QPushButton("Atualizar Selecionados", this);
    m_updateSelectedBtn->setObjectName("updateSelectedButton");
    m_updateSelectedBtn->setEnabled(false);
    m_updateInfoLabel = new QLabel;
    m_updateInfoLabel->setObjectName("updateInfoLabel");

    updateBtnRow->addWidget(m_checkUpdatesBtn);
    updateBtnRow->addWidget(m_updateSelectedBtn);
    updateBtnRow->addWidget(m_updateInfoLabel);
    updateBtnRow->addStretch();
    updateLayout->addLayout(updateBtnRow);

    m_modeStack->addWidget(updatePage);

    mainLayout->addWidget(m_modeStack);
}

void MainWindow::setupSharedFilters(QVBoxLayout *mainLayout)
{
    auto *filterWidget = new QWidget;
    filterWidget->setObjectName("filterWidget");
    auto *filterLayout = new QHBoxLayout(filterWidget);
    filterLayout->setContentsMargins(16, 4, 16, 8);
    filterLayout->setSpacing(6);

    m_filterPacman = new QPushButton("Pacman", this);
    m_filterAur = new QPushButton("AUR", this);
    m_filterFlatpak = new QPushButton("Flatpak", this);

    m_filterPacman->setCheckable(true);
    m_filterAur->setCheckable(true);
    m_filterFlatpak->setCheckable(true);
    m_filterPacman->setChecked(true);
    m_filterAur->setChecked(true);
    m_filterFlatpak->setChecked(true);
    m_filterPacman->setObjectName("filterButton");
    m_filterAur->setObjectName("filterButton");
    m_filterFlatpak->setObjectName("filterButton");

    m_countLabel = new QLabel("Resultados: 0", this);
    m_countLabel->setObjectName("countLabel");

    filterLayout->addWidget(m_filterPacman);
    filterLayout->addWidget(m_filterAur);
    filterLayout->addWidget(m_filterFlatpak);
    filterLayout->addStretch();
    filterLayout->addWidget(m_countLabel);

    m_filterWidget = filterWidget;
    mainLayout->addWidget(m_filterWidget);
}

void MainWindow::setupContent(QVBoxLayout *mainLayout)
{
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setChildrenCollapsible(false);

    m_resultView = new QListView(this);
    m_resultView->setModel(m_proxyModel);
    m_resultView->setItemDelegate(m_delegate);
    m_resultView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_resultView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_resultView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resultView->setFrameShape(QFrame::NoFrame);
    m_resultView->setMouseTracking(true);
    m_resultView->setMinimumWidth(380);

    auto *shadow = new QGraphicsDropShadowEffect(m_resultView);
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(2, 2);
    m_resultView->setGraphicsEffect(shadow);

    m_detailPanel = new DetailPanel(this);
    m_detailPanel->setMinimumWidth(280);

    splitter->addWidget(m_resultView);
    splitter->addWidget(m_detailPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    m_console = new OutputConsole(this);
    auto *runner = new CommandRunner(this);
    m_coordinator->setRunner(runner);

    mainLayout->addWidget(splitter, 1);
    mainLayout->addWidget(m_console);
}

void MainWindow::setupStatusBar(QVBoxLayout *mainLayout)
{
    auto *statusWidget = new QWidget(this);
    statusWidget->setObjectName("statusWidget");
    statusWidget->setFixedHeight(48);

    auto *statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(16, 4, 16, 4);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setFixedWidth(120);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setTextVisible(false);
    m_progressBar->hide();

    m_progressLabel = new QLabel("Pronto.", this);
    m_progressLabel->setObjectName("statusLabel");

    statusLayout->addWidget(m_progressBar);
    statusLayout->addWidget(m_progressLabel);
    statusLayout->addStretch();

    mainLayout->addWidget(statusWidget);
}

void MainWindow::applyStyles(const QString &theme)
{
    bool dark = (theme == "dark" || (theme.isEmpty() && m_isDark));
    m_isDark = dark;

    static QString darkQss, lightQss;
    if (darkQss.isEmpty()) {
        QFile df(":/styles/dark.qss");
        if (df.open(QFile::ReadOnly | QFile::Text))
            darkQss = QString::fromUtf8(df.readAll());
    }
    if (lightQss.isEmpty()) {
        QFile lf(":/styles/light.qss");
        if (lf.open(QFile::ReadOnly | QFile::Text))
            lightQss = QString::fromUtf8(lf.readAll());
    }

    setStyleSheet(dark ? darkQss : lightQss);

    m_detailPanel->setThemeColors(dark);
    m_console->setThemeColors(dark);
    m_delegate->setThemeColors(dark);
    m_resultView->viewport()->update();
}

void MainWindow::switchToSearchMode()
{
    m_inUpdateMode = false;
    m_modeStack->setCurrentIndex(0);
    m_filterWidget->show();
    m_detailPanel->clear();
    m_detailPanel->setUpdateMode(false);
}

void MainWindow::switchToUpdateMode()
{
    m_inUpdateMode = true;
    m_coordinator->cancelSearch();
    m_model->clear();
    m_totalResults = 0;
    m_countLabel->setText("Resultados: 0");
    m_updateInfoLabel->clear();
    m_updateSelectedBtn->setEnabled(false);
    m_modeStack->setCurrentIndex(1);
    m_filterWidget->hide();
    m_detailPanel->clear();
    m_detailPanel->setUpdateMode(true);
}

void MainWindow::setupConnections()
{
    connect(m_modeTab, &QTabBar::currentChanged, this, [this](int idx) {
        if (idx == 0) switchToSearchMode();
        else switchToUpdateMode();
    });

    connect(m_searchInput, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_searchButton, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::onExportClicked);
    connect(m_installAllButton, &QPushButton::clicked, this, &MainWindow::onInstallAllClicked);

    connect(m_checkUpdatesBtn, &QPushButton::clicked, this, &MainWindow::onCheckUpdatesClicked);
    connect(m_updateSelectedBtn, &QPushButton::clicked, this, [this]() {
        onInstallAllClicked();
    });

    connect(m_filterPacman, &QPushButton::toggled, this, [this]() {
        m_proxyModel->setFilterPacman(m_filterPacman->isChecked());
    });
    connect(m_filterAur, &QPushButton::toggled, this, [this]() {
        m_proxyModel->setFilterAur(m_filterAur->isChecked());
    });
    connect(m_filterFlatpak, &QPushButton::toggled, this, [this]() {
        m_proxyModel->setFilterFlatpak(m_filterFlatpak->isChecked());
    });

    connect(m_resultView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);

    connect(m_detailPanel, &DetailPanel::installRequested, this, [this]() {
        QModelIndexList sel = m_resultView->selectionModel()->selectedIndexes();
        if (sel.size() > 1) {
            onInstallAllClicked();
        } else if (sel.size() == 1) {
            QModelIndex sourceIdx = m_proxyModel->mapToSource(sel.first());
            onInstallClicked(m_coordinator->installCommandAt(sourceIdx.row()));
        } else {
            onInstallClicked(m_detailPanel->currentCommand());
        }
    });
    connect(m_detailPanel, &DetailPanel::uninstallRequested,
            this, &MainWindow::onUninstallClicked);

    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(AppSettings::debounceDelayMs());
    connect(m_searchTimer, &QTimer::timeout, this, &MainWindow::onSearchClicked);

    auto *shortcutSearch = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(shortcutSearch, &QShortcut::activated, this, [this]() {
        if (!m_inUpdateMode) { m_searchInput->setFocus(); m_searchInput->selectAll(); }
    });
    auto *shortcutInstall = new QShortcut(QKeySequence("Ctrl+I"), this);
    connect(shortcutInstall, &QShortcut::activated, this, [this]() {
        if (m_inUpdateMode) m_checkUpdatesBtn->click();
        else m_installAllButton->click();
    });
    auto *shortcutClear = new QShortcut(QKeySequence("Ctrl+L"), this);
    connect(shortcutClear, &QShortcut::activated, m_clearButton, &QPushButton::click);
    auto *shortcutQuit = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(shortcutQuit, &QShortcut::activated, this, &QMainWindow::close);
    auto *shortcutExport = new QShortcut(QKeySequence("Ctrl+E"), this);
    connect(shortcutExport, &QShortcut::activated, m_exportButton, &QPushButton::click);

    connect(m_coordinator, &SearchCoordinator::statusChanged,
            this, &MainWindow::onCoordinatorStatus);
    connect(m_coordinator, &SearchCoordinator::resultsAdded,
            this, &MainWindow::onCoordinatorResultsAdded);
    connect(m_coordinator, &SearchCoordinator::crossReferenceStarted,
            this, &MainWindow::onCrossReferenceStarted);
    connect(m_coordinator, &SearchCoordinator::searchFinished,
            this, &MainWindow::onSearchFinished);

    connect(m_coordinator, &SearchCoordinator::progressChanged,
            this, &MainWindow::onCommandProgress);
    connect(m_coordinator, &SearchCoordinator::progressBarVisible,
            this, &MainWindow::onCommandProgressBar);
    connect(m_coordinator, &SearchCoordinator::commandFinished,
            this, &MainWindow::onCommandFinished);
    connect(m_coordinator, &SearchCoordinator::allCommandsFinished,
            this, &MainWindow::onAllCommandsFinished);

    connect(m_coordinator, &SearchCoordinator::processAttached,
            m_console, [this](QProcess *proc) { m_console->attachProcess(proc); });
    connect(m_coordinator, &SearchCoordinator::consoleMessage,
            m_console, [this](const QString &text, int type) {
        if (type == 1) m_console->appendSuccess(text);
        else if (type == 2) m_console->appendError(text);
        else m_console->appendInfo(text);
    });

    connect(m_themeButton, &QPushButton::clicked, this, &MainWindow::onThemeToggle);
    {
        SoundManager &sm = SoundManager::instance();
        m_soundButton->setText(sm.isEnabled()
            ? QString::fromUtf8("\xF0\x9F\x94\x8A")
            : QString::fromUtf8("\xF0\x9F\x94\x87"));
        m_soundButton->setToolTip(sm.isEnabled() ? "Desativar som" : "Ativar som");
        connect(m_soundButton, &QPushButton::clicked, this, [this]() {
            SoundManager &sm = SoundManager::instance();
            sm.toggle();
        });
        connect(&sm, &SoundManager::toggled, this, [this](bool enabled) {
            m_soundButton->setText(enabled
                ? QString::fromUtf8("\xF0\x9F\x94\x8A")
                : QString::fromUtf8("\xF0\x9F\x94\x87"));
            m_soundButton->setToolTip(enabled ? "Desativar som" : "Ativar som");
        });
    }
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    connect(m_minButton, &QPushButton::clicked, this, &QMainWindow::showMinimized);
    connect(m_maxButton, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) showNormal(); else showMaximized();
    });
    connect(m_closeButton, &QPushButton::clicked, this, &QMainWindow::close);
}

void MainWindow::onCheckUpdatesClicked()
{
    m_coordinator->cancelSearch();
    m_model->clear();
    m_totalResults = 0;
    m_detailPanel->clear();
    m_countLabel->setText("Verificando atualizações...");
    m_checkUpdatesBtn->setEnabled(false);
    m_updateInfoLabel->setText("Verificando Pacman, AUR e Flatpak...");

    disconnect(m_updatesWatcher, &QFutureWatcher<QList<SearchItem>>::finished, this, nullptr);
    connect(m_updatesWatcher, &QFutureWatcher<QList<SearchItem>>::finished, this, [this]() {
        if (m_updatesWatcher->isCanceled())
            return;
        QList<SearchItem> updates = m_updatesWatcher->result();
        m_model->addItems(updates);
        m_totalResults = updates.size();
        m_countLabel->setText(QString("Atualizações: %1").arg(m_totalResults));
        m_checkUpdatesBtn->setEnabled(true);
        m_updateInfoLabel->setText(m_totalResults > 0
            ? QString("%1 atualizações disponíveis").arg(m_totalResults)
            : "Sistema atualizado.");
    });

    m_updatesWatcher->setFuture(QtConcurrent::run([this]() {
        QList<SearchItem> all;
        all << m_coordinator->engine()->checkPacmanUpdates();
        all << m_coordinator->engine()->checkAurUpdates();
        all << m_coordinator->engine()->checkFlatpakUpdates();
        return all;
    }));
}

void MainWindow::onSearchClicked()
{
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty()) return;
    m_coordinator->saveSearchHistory(query);
    m_coordinator->search(query);
}

void MainWindow::onInstallClicked(const Command &command)
{
    if (command.isEmpty()) return;

    const QString commandText = command.toString();
    auto reply = QMessageBox::question(this,
        m_inUpdateMode ? "Confirmar Atualização" : "Confirmar Instalação",
        QString("%1?\n\nComando: %2")
            .arg(m_inUpdateMode ? "Atualizar pacote" : "Instalar pacote", commandText),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_console->show();
        m_console->appendInfo(QString("Executando: %1").arg(commandText));
        m_coordinator->runCommand(command, m_inUpdateMode ? "atualização" : "instalação");
    }
}

void MainWindow::onUninstallClicked(const Command &command)
{
    if (command.isEmpty()) return;

    const QString commandText = command.toString();
    auto reply = QMessageBox::question(this, "Confirmar Desinstalação",
        QString("Tem certeza que deseja desinstalar?\n\nComando: %1").arg(commandText),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_console->show();
        m_console->appendInfo(QString("Desinstalando: %1").arg(commandText));
        m_coordinator->runCommand(command, "desinstalação");
    }
}

void MainWindow::onInstallAllClicked()
{
    auto sel = m_resultView->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) {
        if (m_inUpdateMode) {
            QMessageBox::information(this, "Seleção",
                "Selecione um ou mais pacotes para atualizar na lista.");
        } else {
            QMessageBox::information(this, "Seleção",
                "Selecione um ou mais pacotes na lista primeiro.\n"
                "Use Ctrl+Click para múltipla seleção.");
        }
        return;
    }

    QList<SearchItem> items;
    for (const auto &proxyIdx : sel) {
        QModelIndex src = m_proxyModel->mapToSource(proxyIdx);
        items.append(m_coordinator->itemAt(src.row()));
    }

    if (items.size() == 1) {
        onInstallClicked(items.first().installCommand());
        return;
    }

    m_console->show();
    m_console->appendInfo(m_inUpdateMode
        ? QString("Atualizando %1 pacote(s)...").arg(items.size())
        : QString("Instalando %1 pacote(s)...").arg(items.size()));

    const QList<Command> cmds = m_coordinator->buildInstallCommands(items);
    m_coordinator->runCommandsSequentially(cmds,
        m_inUpdateMode ? "atualização" : "instalação");
}

void MainWindow::onClearClicked()
{
    m_coordinator->cancelSearch();
    m_detailPanel->clear();
    m_countLabel->setText("Resultados: 0");
    m_progressLabel->setText("Resultados limpos.");
    if (!m_inUpdateMode) {
        m_searchInput->clear();
        m_searchInput->setFocus();
    }
    m_console->clear();
    m_console->hide();
}

void MainWindow::onThemeToggle()
{
    m_isDark = !m_isDark;
    applyStyles(m_isDark ? "dark" : "light");
    m_themeButton->setText(m_isDark
        ? QString::fromUtf8("\xE2\x98\xBC")
        : QString::fromUtf8("\xE2\x98\x80"));
    AppSettings::setTheme(m_isDark ? "dark" : "light");
    SoundManager::instance().playClick();
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dialog(m_isDark, this);
    connect(&dialog, &SettingsDialog::settingsChanged, this, [this]() {
        m_isDark = AppSettings::resolveDarkTheme(AppSettings::theme(), m_isDark);
        applyStyles(m_isDark ? "dark" : "light");
        m_themeButton->setText(m_isDark
            ? QString::fromUtf8("\xE2\x98\xBC")
            : QString::fromUtf8("\xE2\x98\x80"));
        SoundManager::instance().reloadSettings();
        m_searchTimer->setInterval(AppSettings::debounceDelayMs());
        m_console->setMaximumHeight(AppSettings::consoleMaxHeight());
    });
    dialog.exec();
}

void MainWindow::onCoordinatorStatus(const QString &text)
{
    m_progressLabel->setText(text);
}

void MainWindow::onCoordinatorResultsAdded(int count, int pending, int total)
{
    Q_UNUSED(count)
    Q_UNUSED(pending)
    m_countLabel->setText(QString("Resultados: %1").arg(total));
}

void MainWindow::onCrossReferenceStarted()
{
    m_progressLabel->setText("Fazendo cross-reference...");
}

void MainWindow::onSearchFinished(int total)
{
    m_countLabel->setText(QString("Resultados: %1").arg(total));
    m_progressLabel->setText(total > 0
        ? QString("Busca concluída — %1 resultado(s).").arg(total)
        : "Nenhum resultado encontrado.");
}

void MainWindow::onCommandProgress(const QString &text)
{
    m_progressLabel->setText(text);
}

void MainWindow::onCommandProgressBar(bool visible)
{
    if (visible) m_progressBar->show(); else m_progressBar->hide();
}

void MainWindow::onCommandFinished(bool success, const QString &action)
{
    if (!m_coordinator->isSearching())
        m_progressBar->hide();
    m_progressLabel->setText(
        success ? QString("%1 concluída com sucesso.").arg(action)
                : QString("Erro na %1.").arg(action));
    if (success)
        SoundManager::instance().playSuccess();
    else
        SoundManager::instance().playError();
}

void MainWindow::onAllCommandsFinished(const QString &overallLabel)
{
    m_progressBar->hide();
    m_progressLabel->setText(QString("%1 concluído.").arg(overallLabel));
    m_coordinator->refreshInstalledStatus();
    SoundManager::instance().playSuccess();
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        m_coordinator->cancelSearch();
        m_countLabel->setText("Resultados: 0");
        return;
    }
    m_searchTimer->start();
}

void MainWindow::onSelectionChanged()
{
    QModelIndexList sel = m_resultView->selectionModel()->selectedIndexes();
    int count = sel.size();
    m_detailPanel->setSelectedCount(count);
    if (m_inUpdateMode)
        m_updateSelectedBtn->setEnabled(count > 0);
    if (count == 1) {
        QModelIndex sourceIdx = m_proxyModel->mapToSource(sel.first());
        const SearchItem item = m_coordinator->itemAt(sourceIdx.row());
        m_detailPanel->showPackage(item, m_coordinator->uninstallCommandFor(item));
    }
}

void MainWindow::onExportClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
        "Exportar Resultados", QString(),
        "JSON (*.json);;CSV (*.csv);;Texto (*.txt)");
    if (!filePath.isEmpty())
        exportResults(filePath);
}

void MainWindow::exportResults(const QString &filePath)
{
    QString error;
    if (!ExportService::exportResults(m_model->allItems(), filePath, &error))
        QMessageBox::warning(this, "Erro", error);
    else
        m_progressLabel->setText("Resultados exportados com sucesso.");
}

static const int RESIZE_MARGIN = 15;

static int edgeAtPos(const QRect &r, const QPoint &pos)
{
    int edge = 0;
    if (pos.x() <= r.left() + RESIZE_MARGIN) edge |= 1;
    if (pos.x() >= r.right() - RESIZE_MARGIN) edge |= 2;
    if (pos.y() <= r.top() + RESIZE_MARGIN) edge |= 4;
    if (pos.y() >= r.bottom() - RESIZE_MARGIN) edge |= 8;
    return edge;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            m_resizeEdge = edgeAtPos(geometry(), me->globalPosition().toPoint());
            if (m_resizeEdge) {
                m_dragging = false;
                m_resizeStartPos = me->globalPosition().toPoint();
                m_resizeStartGeo = geometry();
                return true;
            }
            if ((obj == m_titleBar || obj == m_titleLabel || obj == this) &&
                me->globalPosition().toPoint().y() - geometry().top() <= m_titleBar->height()) {
                m_dragging = true;
                m_dragPosition = me->globalPosition().toPoint() - geometry().topLeft();
                return true;
            }
        }
    }
    if (event->type() == QEvent::MouseMove) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (m_resizeEdge) {
            QPoint diff = me->globalPosition().toPoint() - m_resizeStartPos;
            QRect r = m_resizeStartGeo;
            if (m_resizeEdge & 1) { r.setLeft(r.left() + diff.x()); }
            if (m_resizeEdge & 2) { r.setRight(r.right() + diff.x()); }
            if (m_resizeEdge & 4) { r.setTop(r.top() + diff.y()); }
            if (m_resizeEdge & 8) { r.setBottom(r.bottom() + diff.y()); }
            if (r.width() >= minimumWidth() && r.height() >= minimumHeight())
                setGeometry(r);
            return true;
        }
        if (m_dragging) {
            move(me->globalPosition().toPoint() - m_dragPosition);
            return true;
        }
        if (!isMaximized()) {
            int edge = edgeAtPos(geometry(), me->globalPosition().toPoint());
            if (edge == 1 || edge == 2)
                setCursor(Qt::SizeHorCursor);
            else if (edge == 4 || edge == 8)
                setCursor(Qt::SizeVerCursor);
            else if (edge == 5 || edge == 9)
                setCursor(Qt::SizeFDiagCursor);
            else if (edge == 6 || edge == 10)
                setCursor(Qt::SizeBDiagCursor);
            else
                setCursor(Qt::ArrowCursor);
        }
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        m_dragging = false;
        m_resizeEdge = 0;
        setCursor(Qt::ArrowCursor);
    }
    return QMainWindow::eventFilter(obj, event);
}
