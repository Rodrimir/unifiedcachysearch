#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <QObject>
#include <QList>
#include <QSet>
#include <QProcess>
#include <QMutex>
#include "core/searchitem.h"

class SearchEngine : public QObject
{
    Q_OBJECT

public:
    explicit SearchEngine(QObject *parent = nullptr);

    QList<SearchItem> searchPacman(const QString &query);
    QList<SearchItem> searchAur(const QString &query);
    QList<SearchItem> searchFlatpak(const QString &query);

    SearchItem queryPacmanPackageInfo(const QString &name);
    SearchItem queryAurPackageInfo(const QString &name);
    void cancelSearch();

    QString aurHelper() const;
    bool hasFlatpak() const;

    static QString pacmanInstallCmd(const QString &package);
    static QString aurInstallCmd(const QString &helper, const QString &package);
    static QString flatpakInstallCmd(const QString &remote, const QString &appId);
    static QString uninstallCmd(const QString &packageName, SearchItem::Repository type);

    QList<SearchItem> checkPacmanUpdates();
    QList<SearchItem> checkAurUpdates();
    QList<SearchItem> checkFlatpakUpdates();

    static int computeRelevance(const QString &name, const QString &description,
                                 const QString &query);
    static QList<SearchItem> filterByRelevance(QList<SearchItem> items,
                                                const QString &query,
                                                int minScore = 15);

private:
    QString m_aurHelper;
    bool m_hasFlatpak;
    QList<QProcess*> m_activeProcesses;
    QMutex m_processMutex;
    QSet<QString> m_installedPkgs;
    QSet<QString> m_installedFlatpaks;
    mutable QMutex m_pkgMutex;
    QString detectAurHelper();
    bool checkFlatpak();
    QStringList splitQuery(const QString &query);

    QString runProcess(const QString &bin, const QStringList &args, int timeoutMs);

public:
    void refreshInstalledPkgs();
    void refreshInstalledFlatpaks();
    bool isPackageInstalled(const QString &name) const;
    bool isFlatpakInstalled(const QString &name) const;

    QList<SearchItem> parsePacmanOutput(const QString &output);
    QList<SearchItem> parseAurOutput(const QString &output);
    QList<SearchItem> parseFlatpakOutput(const QString &output);
    SearchItem parseSiOutput(const QString &output, SearchItem::Repository type);
};

#endif
