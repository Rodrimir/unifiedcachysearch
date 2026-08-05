#include "ui/settingsdialog.h"
#include "audio/soundmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QtMath>

static double sliderToVolume(int sliderVal) { return qPow(sliderVal / 100.0, 2.0); }
static int volumeToSlider(double vol) { return qRound(qSqrt(vol) * 100.0); }

SettingsDialog::SettingsDialog(bool currentDark, QWidget *parent)
    : QDialog(parent)
    , m_settings("CachyOS", "UnifiedSearch")
    , m_initialDark(currentDark)
{
    setWindowTitle("Configurações");
    setMinimumSize(520, 420);
    setModal(true);

    auto *mainLayout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this);

    setupGeneralTab(tabs);
    setupSearchTab(tabs);
    setupInterfaceTab(tabs);
    setupAboutTab(tabs);

    mainLayout->addWidget(tabs);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        emit settingsChanged();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttons);

    loadSettings();
}

bool SettingsDialog::isDarkTheme() const
{
    QString choice = m_themeCombo->currentData().toString();
    if (choice == "dark") return true;
    if (choice == "light") return false;
    return m_initialDark;
}

static QWidget *makeVolumeRow(const QString &label, QSlider *&slider,
                              QLabel *&valueLabel, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *lay = new QHBoxLayout(row);
    lay->setContentsMargins(0, 0, 0, 0);

    auto *lbl = new QLabel(label, row);
    lbl->setFixedWidth(100);
    lay->addWidget(lbl);

    slider = new QSlider(Qt::Horizontal, row);
    slider->setRange(0, 100);
    slider->setFixedWidth(200);
    lay->addWidget(slider);

    valueLabel = new QLabel("80%", row);
    valueLabel->setFixedWidth(36);
    lay->addWidget(valueLabel);

    lay->addStretch();
    return row;
}

void SettingsDialog::setupGeneralTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *themeGroup = new QGroupBox("Tema", widget);
    auto *themeForm = new QFormLayout(themeGroup);
    m_themeCombo = new QComboBox(widget);
    m_themeCombo->addItem("Sistema (detectar automaticamente)", "system");
    m_themeCombo->addItem("Escuro", "dark");
    m_themeCombo->addItem("Claro", "light");
    themeForm->addRow("Tema:", m_themeCombo);
    layout->addWidget(themeGroup);

    auto *soundGroup = new QGroupBox("Som", widget);
    auto *soundLayout = new QVBoxLayout(soundGroup);

    m_soundCheck = new QCheckBox("Ativar sons da interface", widget);
    soundLayout->addWidget(m_soundCheck);

    soundLayout->addWidget(makeVolumeRow("Sucesso:", m_volSliderSuccess, m_volLabelSuccess, widget));
    soundLayout->addWidget(makeVolumeRow("Erro:", m_volSliderError, m_volLabelError, widget));
    soundLayout->addWidget(makeVolumeRow("Clique:", m_volSliderClick, m_volLabelClick, widget));

    layout->addWidget(soundGroup);
    layout->addStretch();
    tabs->addTab(widget, "Geral");
}

void SettingsDialog::setupSearchTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *timeoutGroup = new QGroupBox("Timeouts de pesquisa (ms)", widget);
    auto *timeoutForm = new QFormLayout(timeoutGroup);

    m_timeoutPacman = new QSpinBox(widget);
    m_timeoutPacman->setRange(3000, 120000);
    m_timeoutPacman->setSingleStep(1000);
    m_timeoutPacman->setSuffix(" ms");
    timeoutForm->addRow("Pacman:", m_timeoutPacman);

    m_timeoutAur = new QSpinBox(widget);
    m_timeoutAur->setRange(3000, 120000);
    m_timeoutAur->setSingleStep(1000);
    m_timeoutAur->setSuffix(" ms");
    timeoutForm->addRow("AUR:", m_timeoutAur);

    m_timeoutFlatpak = new QSpinBox(widget);
    m_timeoutFlatpak->setRange(3000, 120000);
    m_timeoutFlatpak->setSingleStep(1000);
    m_timeoutFlatpak->setSuffix(" ms");
    timeoutForm->addRow("Flatpak:", m_timeoutFlatpak);

    layout->addWidget(timeoutGroup);

    auto *miscGroup = new QGroupBox("Comportamento", widget);
    auto *miscForm = new QFormLayout(miscGroup);

    m_historyLimit = new QSpinBox(widget);
    m_historyLimit->setRange(5, 200);
    m_historyLimit->setSuffix(" itens");
    miscForm->addRow("Limite do histórico:", m_historyLimit);

    m_minScore = new QSpinBox(widget);
    m_minScore->setRange(1, 100);
    miscForm->addRow("Threshold de relevância:", m_minScore);

    m_debounceDelay = new QSpinBox(widget);
    m_debounceDelay->setRange(50, 2000);
    m_debounceDelay->setSingleStep(50);
    m_debounceDelay->setSuffix(" ms");
    miscForm->addRow("Atraso da digitação:", m_debounceDelay);

    m_crossRefSpeed = new QComboBox(widget);
    m_crossRefSpeed->addItem("Mínimo (menos CPU)", 0);
    m_crossRefSpeed->addItem("Normal", 1);
    m_crossRefSpeed->addItem("Máximo (faça meu CPU sofrer)", 2);
    miscForm->addRow("Cross-reference:", m_crossRefSpeed);

    layout->addWidget(miscGroup);
    layout->addStretch();
    tabs->addTab(widget, "Busca");
}

void SettingsDialog::setupInterfaceTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *consoleGroup = new QGroupBox("Console", widget);
    auto *consoleForm = new QFormLayout(consoleGroup);

    m_consoleMaxHeight = new QSpinBox(widget);
    m_consoleMaxHeight->setRange(80, 600);
    m_consoleMaxHeight->setSuffix(" px");
    consoleForm->addRow("Altura máxima:", m_consoleMaxHeight);

    layout->addWidget(consoleGroup);
    layout->addStretch();
    tabs->addTab(widget, "Interface");
}

void SettingsDialog::setupAboutTab(QTabWidget *tabs)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *info = new QLabel(widget);
    info->setWordWrap(true);
    info->setTextFormat(Qt::RichText);
    info->setText(QString(
        "<h2>cachySearch</h2>"
        "<p><b>Versão:</b> %1</p>"
        "<p>Uma ferramenta de busca unificada para Arch Linux / CachyOS, "
        "integrando pacotes Pacman, AUR e Flatpak em uma única interface.</p>"
        "<hr>"
        "<p><b>Desenvolvido com:</b> Qt %2 (compilado %3)</p>"
    ).arg(APP_VERSION,
          QT_VERSION_STR,
          __DATE__));
    info->setAlignment(Qt::AlignCenter);
    layout->addWidget(info);

    layout->addStretch();
    tabs->addTab(widget, "Sobre");
}

void SettingsDialog::loadSettings()
{
    int val;

    QString theme = m_settings.value("theme", "system").toString();
    int idx = m_themeCombo->findData(theme);
    if (idx >= 0) m_themeCombo->setCurrentIndex(idx);

    m_soundCheck->setChecked(m_settings.value("soundEnabled", true).toBool());

    val = volumeToSlider(m_settings.value("soundVolumeSuccess", 0.8).toDouble());
    m_volSliderSuccess->setValue(val);
    m_volLabelSuccess->setText(QString("%1%").arg(val));

    val = volumeToSlider(m_settings.value("soundVolumeError", 0.7).toDouble());
    m_volSliderError->setValue(val);
    m_volLabelError->setText(QString("%1%").arg(val));

    val = volumeToSlider(m_settings.value("soundVolumeClick", 0.6).toDouble());
    m_volSliderClick->setValue(val);
    m_volLabelClick->setText(QString("%1%").arg(val));

    m_timeoutPacman->setValue(m_settings.value("pacmanTimeout", 15000).toInt());
    m_timeoutAur->setValue(m_settings.value("aurTimeout", 30000).toInt());
    m_timeoutFlatpak->setValue(m_settings.value("flatpakTimeout", 15000).toInt());
    m_historyLimit->setValue(m_settings.value("historyLimit", 20).toInt());
    m_minScore->setValue(m_settings.value("minScore", 15).toInt());
    m_debounceDelay->setValue(m_settings.value("debounceDelay", 400).toInt());

    m_consoleMaxHeight->setValue(m_settings.value("consoleMaxHeight", 280).toInt());

    int cref = m_settings.value("crossRefSpeed", 1).toInt();
    int cidx = m_crossRefSpeed->findData(cref);
    if (cidx >= 0) m_crossRefSpeed->setCurrentIndex(cidx);

    connect(m_volSliderSuccess, &QSlider::valueChanged, this, [this](int v) {
        m_volLabelSuccess->setText(QString("%1%").arg(v));
    });
    connect(m_volSliderSuccess, &QSlider::sliderReleased, this, [this]() {
        SoundManager::instance().playPreview("success", sliderToVolume(m_volSliderSuccess->value()));
    });
    connect(m_volSliderError, &QSlider::valueChanged, this, [this](int v) {
        m_volLabelError->setText(QString("%1%").arg(v));
    });
    connect(m_volSliderError, &QSlider::sliderReleased, this, [this]() {
        SoundManager::instance().playPreview("error", sliderToVolume(m_volSliderError->value()));
    });
    connect(m_volSliderClick, &QSlider::valueChanged, this, [this](int v) {
        m_volLabelClick->setText(QString("%1%").arg(v));
    });
    connect(m_volSliderClick, &QSlider::sliderReleased, this, [this]() {
        SoundManager::instance().playPreview("click", sliderToVolume(m_volSliderClick->value()));
    });
}

void SettingsDialog::saveSettings()
{
    m_settings.setValue("theme", m_themeCombo->currentData().toString());
    m_settings.setValue("soundEnabled", m_soundCheck->isChecked());
    m_settings.setValue("soundVolumeSuccess", sliderToVolume(m_volSliderSuccess->value()));
    m_settings.setValue("soundVolumeError", sliderToVolume(m_volSliderError->value()));
    m_settings.setValue("soundVolumeClick", sliderToVolume(m_volSliderClick->value()));

    m_settings.setValue("pacmanTimeout", m_timeoutPacman->value());
    m_settings.setValue("aurTimeout", m_timeoutAur->value());
    m_settings.setValue("flatpakTimeout", m_timeoutFlatpak->value());
    m_settings.setValue("historyLimit", m_historyLimit->value());
    m_settings.setValue("minScore", m_minScore->value());
    m_settings.setValue("debounceDelay", m_debounceDelay->value());

    m_settings.setValue("consoleMaxHeight", m_consoleMaxHeight->value());
    m_settings.setValue("crossRefSpeed", m_crossRefSpeed->currentData().toInt());
}
