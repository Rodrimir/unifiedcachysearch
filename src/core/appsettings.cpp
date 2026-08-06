#include "core/appsettings.h"

namespace {

// Defaults centralizados: a tela de configuracoes e os consumidores leem daqui.
constexpr double DefaultVolumeSuccess = 0.8;
constexpr double DefaultVolumeError   = 0.7;
constexpr double DefaultVolumeClick   = 0.6;
constexpr int DefaultPacmanTimeout    = 15000;
constexpr int DefaultAurTimeout       = 30000;
constexpr int DefaultFlatpakTimeout   = 15000;
constexpr int DefaultHistoryLimit     = 20;
constexpr int DefaultMinScore         = 15;
constexpr int DefaultDebounceDelay    = 400;
constexpr int DefaultCrossRefSpeed    = 1;
constexpr int DefaultConsoleMaxHeight = 280;

} // namespace

namespace AppSettings {

QSettings open()
{
    return QSettings(Organization, Application);
}

QString theme()
{
    return open().value(Keys::Theme, "system").toString();
}

void setTheme(const QString &theme)
{
    open().setValue(Keys::Theme, theme);
}

bool resolveDarkTheme(const QString &theme, bool systemIsDark)
{
    if (theme == "dark")  return true;
    if (theme == "light") return false;
    return systemIsDark;
}

bool soundEnabled()
{
    return open().value(Keys::SoundEnabled, true).toBool();
}

void setSoundEnabled(bool enabled)
{
    open().setValue(Keys::SoundEnabled, enabled);
}

double soundVolumeSuccess()
{
    return open().value(Keys::SoundVolumeSuccess, DefaultVolumeSuccess).toDouble();
}

double soundVolumeError()
{
    return open().value(Keys::SoundVolumeError, DefaultVolumeError).toDouble();
}

double soundVolumeClick()
{
    return open().value(Keys::SoundVolumeClick, DefaultVolumeClick).toDouble();
}

int pacmanTimeoutMs()
{
    return open().value(Keys::PacmanTimeout, DefaultPacmanTimeout).toInt();
}

int aurTimeoutMs()
{
    return open().value(Keys::AurTimeout, DefaultAurTimeout).toInt();
}

int flatpakTimeoutMs()
{
    return open().value(Keys::FlatpakTimeout, DefaultFlatpakTimeout).toInt();
}

int historyLimit()
{
    return open().value(Keys::HistoryLimit, DefaultHistoryLimit).toInt();
}

int minRelevanceScore()
{
    return open().value(Keys::MinScore, DefaultMinScore).toInt();
}

int debounceDelayMs()
{
    return open().value(Keys::DebounceDelay, DefaultDebounceDelay).toInt();
}

int crossRefSpeed()
{
    return open().value(Keys::CrossRefSpeed, DefaultCrossRefSpeed).toInt();
}

int consoleMaxHeight()
{
    return open().value(Keys::ConsoleMaxHeight, DefaultConsoleMaxHeight).toInt();
}

QStringList searchHistory()
{
    return open().value(Keys::SearchHistory).toStringList();
}

void setSearchHistory(const QStringList &history)
{
    open().setValue(Keys::SearchHistory, history);
}

} // namespace AppSettings
