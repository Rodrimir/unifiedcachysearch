#include "ui/settingsdialog.h"
#include "audio/soundmanager.h"
#include "core/appsettings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QtMath>

static double sliderToVolume(int sliderVal) { return qPow(sliderVal / 100.0, 2.0); }
static int volumeToSlider(double vol) { return qRound(qSqrt(vol) * 100.0); }

SettingsDialog::SettingsDialog(bool currentDark, QWidget *parent)
    : QDialog(parent)
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
    // Valores e defaults vem do AppSettings, os mesmos que o resto do app le.
    const int themeIdx = m_themeCombo->findData(AppSettings::theme());
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);

    m_soundCheck->setChecked(AppSettings::soundEnabled());

    auto applyVolume = [](QSlider *slider, QLabel *label, double volume) {
        const int val = volumeToSlider(volume);
        slider->setValue(val);
        label->setText(QString("%1%").arg(val));
    };
    applyVolume(m_volSliderSuccess, m_volLabelSuccess, AppSettings::soundVolumeSuccess());
    applyVolume(m_volSliderError, m_volLabelError, AppSettings::soundVolumeError());
    applyVolume(m_volSliderClick, m_volLabelClick, AppSettings::soundVolumeClick());

    m_timeoutPacman->setValue(AppSettings::pacmanTimeoutMs());
    m_timeoutAur->setValue(AppSettings::aurTimeoutMs());
    m_timeoutFlatpak->setValue(AppSettings::flatpakTimeoutMs());
    m_historyLimit->setValue(AppSettings::historyLimit());
    m_minScore->setValue(AppSettings::minRelevanceScore());
    m_debounceDelay->setValue(AppSettings::debounceDelayMs());

    m_consoleMaxHeight->setValue(AppSettings::consoleMaxHeight());

    const int crossRefIdx = m_crossRefSpeed->findData(AppSettings::crossRefSpeed());
    if (crossRefIdx >= 0) m_crossRefSpeed->setCurrentIndex(crossRefIdx);

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
    using namespace AppSettings;
    QSettings settings = AppSettings::open();

    settings.setValue(Keys::Theme, m_themeCombo->currentData().toString());
    settings.setValue(Keys::SoundEnabled, m_soundCheck->isChecked());
    settings.setValue(Keys::SoundVolumeSuccess, sliderToVolume(m_volSliderSuccess->value()));
    settings.setValue(Keys::SoundVolumeError, sliderToVolume(m_volSliderError->value()));
    settings.setValue(Keys::SoundVolumeClick, sliderToVolume(m_volSliderClick->value()));

    settings.setValue(Keys::PacmanTimeout, m_timeoutPacman->value());
    settings.setValue(Keys::AurTimeout, m_timeoutAur->value());
    settings.setValue(Keys::FlatpakTimeout, m_timeoutFlatpak->value());
    settings.setValue(Keys::HistoryLimit, m_historyLimit->value());
    settings.setValue(Keys::MinScore, m_minScore->value());
    settings.setValue(Keys::DebounceDelay, m_debounceDelay->value());

    settings.setValue(Keys::ConsoleMaxHeight, m_consoleMaxHeight->value());
    settings.setValue(Keys::CrossRefSpeed, m_crossRefSpeed->currentData().toInt());
}
