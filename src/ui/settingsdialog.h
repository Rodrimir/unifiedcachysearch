#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QDialogButtonBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(bool currentDark, QWidget *parent = nullptr);

    bool isDarkTheme() const;

signals:
    void settingsChanged();

private:
    void setupGeneralTab(QTabWidget *tabs);
    void setupSearchTab(QTabWidget *tabs);
    void setupInterfaceTab(QTabWidget *tabs);
    void setupAboutTab(QTabWidget *tabs);
    void loadSettings();
    void saveSettings();

    // General
    QComboBox *m_themeCombo;
    QCheckBox *m_soundCheck;
    QSlider *m_volSliderSuccess;
    QSlider *m_volSliderError;
    QSlider *m_volSliderClick;
    QLabel *m_volLabelSuccess;
    QLabel *m_volLabelError;
    QLabel *m_volLabelClick;

    // Search
    QSpinBox *m_timeoutPacman;
    QSpinBox *m_timeoutAur;
    QSpinBox *m_timeoutFlatpak;
    QSpinBox *m_historyLimit;
    QSpinBox *m_minScore;
    QSpinBox *m_debounceDelay;
    QComboBox *m_crossRefSpeed;

    // Interface
    QSpinBox *m_consoleMaxHeight;

    bool m_initialDark;
};

#endif
