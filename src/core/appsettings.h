#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QSettings>
#include <QString>
#include <QStringList>

// Ponto unico de acesso as preferencias. Antes cada arquivo abria seu proprio
// QSettings e repetia o valor padrao, o que fazia os defaults divergirem entre
// a tela de configuracoes e quem realmente lia a chave.
namespace AppSettings {

inline constexpr const char *Organization = "CachyOS";
inline constexpr const char *Application  = "UnifiedSearch";

// Nomes das chaves em um lugar so: e assim que "minScore" acabou salvo pela
// tela de configuracoes sem nunca ser lido por ninguem.
namespace Keys {
inline constexpr const char *Theme              = "theme";
inline constexpr const char *SoundEnabled       = "soundEnabled";
inline constexpr const char *SoundVolumeSuccess = "soundVolumeSuccess";
inline constexpr const char *SoundVolumeError   = "soundVolumeError";
inline constexpr const char *SoundVolumeClick   = "soundVolumeClick";
inline constexpr const char *PacmanTimeout      = "pacmanTimeout";
inline constexpr const char *AurTimeout         = "aurTimeout";
inline constexpr const char *FlatpakTimeout     = "flatpakTimeout";
inline constexpr const char *HistoryLimit       = "historyLimit";
inline constexpr const char *MinScore           = "minScore";
inline constexpr const char *DebounceDelay      = "debounceDelay";
inline constexpr const char *CrossRefSpeed      = "crossRefSpeed";
inline constexpr const char *ConsoleMaxHeight   = "consoleMaxHeight";
inline constexpr const char *SearchHistory      = "searchHistory";
} // namespace Keys

QSettings open();

// Tema: "system" | "dark" | "light"
QString theme();
void setTheme(const QString &theme);
bool resolveDarkTheme(const QString &theme, bool systemIsDark);

bool soundEnabled();
void setSoundEnabled(bool enabled);
double soundVolumeSuccess();
double soundVolumeError();
double soundVolumeClick();

int pacmanTimeoutMs();
int aurTimeoutMs();
int flatpakTimeoutMs();

int historyLimit();
int minRelevanceScore();
int debounceDelayMs();
int crossRefSpeed();

int consoleMaxHeight();

QStringList searchHistory();
void setSearchHistory(const QStringList &history);

} // namespace AppSettings

#endif
