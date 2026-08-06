#include "audio/soundmanager.h"
#include "core/appsettings.h"
#include <QTimer>
#include <QUrl>

SoundManager& SoundManager::instance()
{
    static SoundManager inst;
    return inst;
}

SoundManager::SoundManager(QObject *parent)
    : QObject(parent)
{
    m_enabled = AppSettings::soundEnabled();

    m_sfxSuccess = new QSoundEffect(this);
    m_sfxSuccess->setSource(QUrl("qrc:/sounds/success.wav"));
    m_sfxSuccess->setVolume(AppSettings::soundVolumeSuccess());

    m_sfxError = new QSoundEffect(this);
    m_sfxError->setSource(QUrl("qrc:/sounds/error.wav"));
    m_sfxError->setVolume(AppSettings::soundVolumeError());

    m_sfxClick = new QSoundEffect(this);
    m_sfxClick->setSource(QUrl("qrc:/sounds/click.wav"));
    m_sfxClick->setVolume(AppSettings::soundVolumeClick());
}

void SoundManager::playSuccess()
{
    if (m_enabled) m_sfxSuccess->play();
}

void SoundManager::playError()
{
    if (m_enabled) m_sfxError->play();
}

void SoundManager::playClick()
{
    if (m_enabled) m_sfxClick->play();
}

void SoundManager::playPreview(const QString &type, double volume)
{
    double vol = qBound(0.0, volume, 1.0);
    QSoundEffect *fx = nullptr;
    double prevVol = 0.0;

    if (type == "success")      { fx = m_sfxSuccess; prevVol = m_sfxSuccess->volume(); }
    else if (type == "error")   { fx = m_sfxError;   prevVol = m_sfxError->volume();   }
    else if (type == "click")   { fx = m_sfxClick;   prevVol = m_sfxClick->volume();   }

    if (!fx) return;

    fx->setVolume(vol);
    fx->play();

    QTimer::singleShot(100, this, [fx, prevVol]() {
        fx->setVolume(prevVol);
    });
}

void SoundManager::reloadSettings()
{
    m_enabled = AppSettings::soundEnabled();
    m_sfxSuccess->setVolume(AppSettings::soundVolumeSuccess());
    m_sfxError->setVolume(AppSettings::soundVolumeError());
    m_sfxClick->setVolume(AppSettings::soundVolumeClick());
}

void SoundManager::setEnabled(bool e)
{
    m_enabled = e;
    AppSettings::setSoundEnabled(e);
    emit toggled(e);
}

void SoundManager::toggle()
{
    setEnabled(!m_enabled);
}
