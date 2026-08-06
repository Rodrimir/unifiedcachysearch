#include "ui/detailpanel.h"
#include "core/repositoryinfo.h"
#include "core/theme.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

DetailPanel::DetailPanel(QWidget *parent)
    : QWidget(parent)
{
    setThemeColors(true);
    setupUI();
}

void DetailPanel::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_emptyLabel = new QLabel("Selecione um pacote\npara ver detalhes", this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setObjectName("emptyLabel");

    m_contentWidget = new QWidget(this);
    m_contentWidget->setObjectName("detailContent");
    auto *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setSpacing(8);
    contentLayout->setContentsMargins(16, 16, 16, 16);

    auto *headerLayout = new QHBoxLayout();

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(48, 48);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setObjectName("iconLabel");

    auto *nameVersionLayout = new QVBoxLayout();
    m_nameLabel = new QLabel(this);
    m_nameLabel->setObjectName("nameLabel");
    m_versionLabel = new QLabel(this);
    m_versionLabel->setObjectName("versionLabel");

    nameVersionLayout->addWidget(m_nameLabel);
    nameVersionLayout->addWidget(m_versionLabel);
    headerLayout->addWidget(m_iconLabel);
    headerLayout->addLayout(nameVersionLayout);
    headerLayout->addStretch();

    contentLayout->addLayout(headerLayout);

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setObjectName("hSeparator");
    contentLayout->addWidget(separator);

    auto *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(6);

    m_repoLabel = new QLabel(this);
    m_repoLabel->setObjectName("repoLabel");
    infoLayout->addWidget(m_repoLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    infoLayout->addWidget(m_statusLabel);

    contentLayout->addLayout(infoLayout);

    auto *descSeparator = new QFrame(this);
    descSeparator->setFrameShape(QFrame::HLine);
    descSeparator->setObjectName("hSeparator");
    contentLayout->addWidget(descSeparator);

    QLabel *descTitle = new QLabel("Descrição", this);
    descTitle->setObjectName("sectionTitle");
    contentLayout->addWidget(descTitle);

    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setObjectName("descriptionLabel");
    contentLayout->addWidget(m_descriptionLabel);

    auto *cmdSeparator = new QFrame(this);
    cmdSeparator->setFrameShape(QFrame::HLine);
    cmdSeparator->setObjectName("hSeparator");
    contentLayout->addWidget(cmdSeparator);

    m_cmdTitle = new QLabel("Comando de Instalação", this);
    m_cmdTitle->setObjectName("sectionTitle");
    contentLayout->addWidget(m_cmdTitle);

    m_commandLabel = new QLabel(this);
    m_commandLabel->setObjectName("commandLabel");
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLayout->addWidget(m_commandLabel);

    contentLayout->addStretch();

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    m_installButton = new QPushButton("Instalar Pacote", this);
    m_installButton->setObjectName("installPkgButton");

    m_uninstallButton = new QPushButton("Desinstalar", this);
    m_uninstallButton->setObjectName("uninstallPkgButton");
    m_uninstallButton->hide();

    connect(m_installButton, &QPushButton::clicked, this, [this]() {
        if (m_hasPackage)
            emit installRequested();
    });
    connect(m_uninstallButton, &QPushButton::clicked, this, [this]() {
        if (!m_uninstallCommand.isEmpty())
            emit uninstallRequested(m_uninstallCommand);
    });

    buttonLayout->addWidget(m_installButton);
    buttonLayout->addWidget(m_uninstallButton);
    contentLayout->addLayout(buttonLayout);

    mainLayout->addWidget(m_emptyLabel);
    mainLayout->addWidget(m_contentWidget);

    m_contentWidget->hide();
}

void DetailPanel::setThemeColors(bool dark)
{
    m_isDark = dark;

    auto c = dark ? darkTheme() : lightTheme();

    setStyleSheet(QString(R"(
        DetailPanel {
            background-color: %1;
            border-radius: 8px;
        }
        #emptyLabel {
            color: %4;
            font-size: 14px;
            padding: 40px;
        }
        #iconLabel {
            background-color: %2;
            border-radius: 24px;
            font-size: 24px;
        }
        #nameLabel {
            font-size: 18px;
            font-weight: bold;
            color: %3;
        }
        #versionLabel {
            font-size: 13px;
            color: %4;
        }
        #repoLabel, #statusLabel {
            font-size: 13px;
            color: %4;
        }
        .sectionTitle, #sectionTitle {
            font-size: 13px;
            font-weight: bold;
            color: %3;
        }
        #descriptionLabel {
            font-size: 12px;
            color: %4;
            padding: 4px 0;
        }
        #commandLabel {
            font-size: 12px;
            color: #fab387;
            font-family: monospace;
            background-color: %2;
            padding: 8px;
            border-radius: 4px;
        }
        #hSeparator {
            background-color: %5;
            max-height: 1px;
        }
        #installPkgButton {
            background-color: %7;
            color: %2;
            border: none;
            border-radius: 8px;
            padding: 12px 24px;
            font-size: 14px;
            font-weight: bold;
        }
        #installPkgButton:hover {
            background-color: #b8ebc0;
        }
        #installPkgButton:pressed {
            background-color: #94d89a;
        }
        #installPkgButton[installed="true"] {
            background-color: %6;
        }
        #installPkgButton[installed="true"]:hover {
            background-color: #b4d0fb;
        }
        #uninstallPkgButton {
            background-color: %8;
            color: %2;
            border: none;
            border-radius: 8px;
            padding: 12px 24px;
            font-size: 14px;
            font-weight: bold;
        }
        #uninstallPkgButton:hover {
            background-color: #f9b9c9;
        }
        #uninstallPkgButton:pressed {
            background-color: #eb8da8;
        }
    )")
    .arg(c.bg1, c.bg2, c.fg1, c.fg2, c.fg3, c.accent, c.green, c.red));
}

void DetailPanel::updateInstallButtonText()
{
    if (!m_hasPackage)
        return;

    if (m_selectedCount > 1) {
        const QString action = m_updateMode ? "Atualizar" : "Instalar";
        m_installButton->setText(QString("%1 (%2 pacotes)...").arg(action).arg(m_selectedCount));
    } else if (m_updateMode) {
        m_installButton->setText("Atualizar");
    } else {
        // Antes isso era deduzido do texto do label de status.
        m_installButton->setText(m_currentInstalled ? "Reinstalar" : "Instalar Pacote");
    }
}

void DetailPanel::setSelectedCount(int count)
{
    m_selectedCount = count;
    updateInstallButtonText();
}

void DetailPanel::setUpdateMode(bool updateMode)
{
    m_updateMode = updateMode;
    m_cmdTitle->setText(updateMode ? "Comando de Atualização" : "Comando de Instalação");
    updateInstallButtonText();
}

void DetailPanel::showPackage(const SearchItem &item, const Command &uninstallCommand)
{
    m_emptyLabel->hide();
    m_contentWidget->show();

    m_iconLabel->setText(RepositoryInfo::icon(item.repositoryType()));
    m_nameLabel->setText(item.name());
    m_versionLabel->setText(QString("Versão: %1").arg(item.version()));
    m_repoLabel->setText(QString("Repositório: %1 (%2)")
                             .arg(RepositoryInfo::name(item.repositoryType()), item.repository()));

    m_currentInstalled = item.isInstalled();

    if (m_updateMode) {
        m_statusLabel->setText("Status: Atualização disponível");
        m_statusLabel->setStyleSheet(QString(
            "font-size: 13px; font-weight: bold; color: %1;")
            .arg(m_isDark ? "#fab387" : "#fe640b"));
        m_uninstallButton->show();
    } else if (item.isInstalled()) {
        m_statusLabel->setText("Status: Instalado");
        m_statusLabel->setStyleSheet(QString(
            "font-size: 13px; font-weight: bold; color: %1;")
            .arg(m_isDark ? "#a6e3a1" : "#40a02b"));
        m_uninstallButton->show();
    } else {
        m_statusLabel->setText("Status: Não instalado");
        m_statusLabel->setStyleSheet(QString(
            "font-size: 13px; color: %1;")
            .arg(m_isDark ? "#f38ba8" : "#d20f39"));
        m_uninstallButton->hide();
    }

    m_descriptionLabel->setText(item.description().isEmpty()
        ? QStringLiteral("Sem descrição disponível.")
        : item.description());
    m_commandLabel->setText(item.installCommandText());

    m_installCommand = item.installCommand();
    m_uninstallCommand = uninstallCommand;
    m_hasPackage = true;

    updateInstallButtonText();
}

void DetailPanel::clear()
{
    m_contentWidget->hide();
    m_emptyLabel->show();
    m_installCommand = {};
    m_uninstallCommand = {};
    m_hasPackage = false;
    m_currentInstalled = false;
    m_selectedCount = 0;
}
