#ifndef SEARCHITEM_H
#define SEARCHITEM_H

#include <QList>
#include <QString>
#include "core/command.h"

// Um pacote encontrado em qualquer uma das tres fontes.
class SearchItem
{
public:
    enum Repository {
        Pacman,
        AUR,
        Flatpak
    };

    SearchItem() = default;
    SearchItem(const QString &name, const QString &version,
               const QString &repository, const QString &displayName,
               const Command &installCommand)
        : m_name(name)
        , m_version(version)
        , m_repository(repository)
        , m_displayName(displayName)
        , m_installCommand(installCommand)
    {
    }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString version() const { return m_version; }
    void setVersion(const QString &version) { m_version = version; }

    QString repository() const { return m_repository; }
    void setRepository(const QString &repository) { m_repository = repository; }

    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }

    Command installCommand() const { return m_installCommand; }
    void setInstallCommand(const Command &command) { m_installCommand = command; }
    // Forma textual, para exibicao e exportacao.
    QString installCommandText() const { return m_installCommand.toString(); }

    QString description() const { return m_description; }
    void setDescription(const QString &description) { m_description = description; }

    bool isInstalled() const { return m_isInstalled; }
    void setIsInstalled(bool installed) { m_isInstalled = installed; }

    Repository repositoryType() const { return m_repositoryType; }
    void setRepositoryType(Repository type) { m_repositoryType = type; }

    int relevanceScore() const { return m_relevanceScore; }
    void setRelevanceScore(int score) { m_relevanceScore = score; }

private:
    QString m_name;
    QString m_version;
    QString m_repository;
    QString m_displayName;
    Command m_installCommand;
    QString m_description;
    bool m_isInstalled = false;
    Repository m_repositoryType = Pacman;
    int m_relevanceScore = 0;
};

// Resultado do cruzamento entre as tres fontes.
struct CrossReferenceResult {
    QList<SearchItem> pacman;
    QList<SearchItem> aur;
    QList<SearchItem> flatpak;
};

#endif
