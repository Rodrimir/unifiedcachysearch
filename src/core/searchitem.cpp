#include "core/searchitem.h"

SearchItem::SearchItem()
    : m_repositoryType(Pacman)
{
}

SearchItem::SearchItem(const QString &name, const QString &version,
                       const QString &repository, const QString &displayName,
                       const QString &installCommand)
    : m_name(name)
    , m_version(version)
    , m_repository(repository)
    , m_displayName(displayName)
    , m_installCommand(installCommand)
    , m_repositoryType(Pacman)
{
}

QString SearchItem::name() const { return m_name; }
void SearchItem::setName(const QString &name) { m_name = name; }

QString SearchItem::version() const { return m_version; }
void SearchItem::setVersion(const QString &version) { m_version = version; }

QString SearchItem::repository() const { return m_repository; }
void SearchItem::setRepository(const QString &repository) { m_repository = repository; }

QString SearchItem::displayName() const { return m_displayName; }
void SearchItem::setDisplayName(const QString &displayName) { m_displayName = displayName; }

QString SearchItem::installCommand() const { return m_installCommand; }
void SearchItem::setInstallCommand(const QString &installCommand) { m_installCommand = installCommand; }

QString SearchItem::description() const { return m_description; }
void SearchItem::setDescription(const QString &description) { m_description = description; }

bool SearchItem::isInstalled() const { return m_isInstalled; }
void SearchItem::setIsInstalled(bool installed) { m_isInstalled = installed; }

SearchItem::Repository SearchItem::repositoryType() const { return m_repositoryType; }
void SearchItem::setRepositoryType(Repository type) { m_repositoryType = type; }

int SearchItem::relevanceScore() const { return m_relevanceScore; }
void SearchItem::setRelevanceScore(int score) { m_relevanceScore = score; }
