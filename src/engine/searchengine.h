#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <QAtomicInt>
#include <QMutex>
#include <QObject>
#include <QSet>
#include "core/searchitem.h"
#include "engine/commandbuilder.h"
#include "engine/processrunner.h"

// Consulta pacman, o helper do AUR e o flatpak, e mantem o cache do que esta
// instalado. O parsing vive em PackageParser e a construcao de comandos em
// CommandBuilder.
//
// Threading: os metodos de busca sao chamados de tarefas do QtConcurrent;
// cancelSearch() e invalidateInstalledCache() vem da thread da UI.
class SearchEngine : public QObject
{
    Q_OBJECT

public:
    explicit SearchEngine(QObject *parent = nullptr);

    QString aurHelper() const { return m_commands.aurHelper(); }
    bool hasFlatpak() const { return m_hasFlatpak; }
    const CommandBuilder &commands() const { return m_commands; }

    // --- Buscas (threads de trabalho) ---
    QList<SearchItem> searchPacman(const QString &query);
    QList<SearchItem> searchAur(const QString &query);
    QList<SearchItem> searchFlatpak(const QString &query);

    SearchItem queryPacmanPackageInfo(const QString &name);
    SearchItem queryAurPackageInfo(const QString &name);

    QList<SearchItem> checkPacmanUpdates();
    QList<SearchItem> checkAurUpdates();
    QList<SearchItem> checkFlatpakUpdates();

    // --- Estado de instalacao ---
    // Marca o cache como velho; a proxima busca o refaz fora da thread da UI.
    void invalidateInstalledCache();
    void refreshInstalledPackages();
    void refreshInstalledFlatpaks();
    bool isPackageInstalled(const QString &name) const;
    bool isFlatpakInstalled(const QString &name) const;

    // Interrompe os processos em andamento (qualquer thread).
    void cancelSearch();

private:
    void ensureInstalledPackages();
    void ensureInstalledFlatpaks();

    CommandBuilder m_commands;
    bool m_hasFlatpak = false;
    ProcessRunner m_runner;

    QSet<QString> m_installedPkgs;
    QSet<QString> m_installedFlatpaks;
    mutable QMutex m_pkgMutex;    // protege os dois QSet acima
    QMutex m_refreshMutex;        // serializa os refreshes concorrentes
    QAtomicInt m_pkgCacheValid;
    QAtomicInt m_flatpakCacheValid;
};

#endif
