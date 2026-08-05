#ifndef SEARCHITEM_H
#define SEARCHITEM_H

#include <QString>
#include <QList>

class SearchItem
{
public:
    enum Repository {
        Pacman,
        AUR,
        Flatpak
    };

    SearchItem();
    SearchItem(const QString &name, const QString &version,
               const QString &repository, const QString &displayName,
               const QString &installCommand);

    QString name() const;
    void setName(const QString &name);

    QString version() const;
    void setVersion(const QString &version);

    QString repository() const;
    void setRepository(const QString &repository);

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    QString installCommand() const;
    void setInstallCommand(const QString &installCommand);

    QString description() const;
    void setDescription(const QString &description);

    bool isInstalled() const;
    void setIsInstalled(bool installed);

    Repository repositoryType() const;
    void setRepositoryType(Repository type);

    int relevanceScore() const;
    void setRelevanceScore(int score);

private:
    QString m_name;
    QString m_version;
    QString m_repository;
    QString m_displayName;
    QString m_installCommand;
    QString m_description;
    bool m_isInstalled = false;
    Repository m_repositoryType = Pacman;
    int m_relevanceScore = 0;
};

struct CrossReferenceResult {
    QList<SearchItem> pacman;
    QList<SearchItem> aur;
    QList<SearchItem> flatpak;
};

#endif
