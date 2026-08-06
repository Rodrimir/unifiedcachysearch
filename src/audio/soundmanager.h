#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QSoundEffect>

class SoundManager : public QObject
{
    Q_OBJECT

public:
    static SoundManager& instance();

    void playSuccess();
    void playError();
    void playClick();
    void playPreview(const QString &type, double volume);

    void reloadSettings();

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool e);
    void toggle();

signals:
    void toggled(bool enabled);

private:
    explicit SoundManager(QObject *parent = nullptr);

    QSoundEffect *m_sfxSuccess;
    QSoundEffect *m_sfxError;
    QSoundEffect *m_sfxClick;
    bool m_enabled = true;
};

#endif
