#include "audio/soundmanager.h"
#include <QUrl>
#include <QTimer>

SoundManager& SoundManager::instance()
{
    static SoundManager inst;
    return inst;
}

SoundManager::SoundManager(QObject *parent)
    : QObject(parent)
{
    QSettings s("CachyOS", "UnifiedSearch");
    m_enabled = s.value("soundEnabled", true).toBool();

    m_sfxSuccess = new QSoundEffect(this);
    m_sfxSuccess->setSource(QUrl("qrc:/sounds/success.wav"));
    m_sfxSuccess->setVolume(s.value("soundVolumeSuccess", 0.8).toDouble());

    m_sfxError = new QSoundEffect(this);
    m_sfxError->setSource(QUrl("qrc:/sounds/error.wav"));
    m_sfxError->setVolume(s.value("soundVolumeError", 0.7).toDouble());

    m_sfxClick = new QSoundEffect(this);
    m_sfxClick->setSource(QUrl("qrc:/sounds/click.wav"));
    m_sfxClick->setVolume(s.value("soundVolumeClick", 0.6).toDouble());
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
    QSettings s("CachyOS", "UnifiedSearch");
    m_enabled = s.value("soundEnabled", true).toBool();
    m_sfxSuccess->setVolume(s.value("soundVolumeSuccess", 0.8).toDouble());
    m_sfxError->setVolume(s.value("soundVolumeError", 0.7).toDouble());
    m_sfxClick->setVolume(s.value("soundVolumeClick", 0.6).toDouble());
}

void SoundManager::setEnabled(bool e)
{
    m_enabled = e;
    QSettings("CachyOS", "UnifiedSearch").setValue("soundEnabled", e);
    emit toggled(e);
}

void SoundManager::toggle()
{
    setEnabled(!m_enabled);
}
