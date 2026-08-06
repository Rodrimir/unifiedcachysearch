#ifndef COMMANDBUILDER_H
#define COMMANDBUILDER_H

#include <QString>
#include <QStringList>
#include "core/command.h"
#include "core/searchitem.h"

// Constroi os comandos de instalacao/remocao. Fica separado do SearchEngine
// para que o parsing e os testes nao precisem de nenhum processo externo.
class CommandBuilder
{
public:
    CommandBuilder() = default;
    explicit CommandBuilder(const QString &aurHelper);

    QString aurHelper() const { return m_aurHelper; }
    bool hasAurHelper() const { return !m_aurHelper.isEmpty(); }

    Command installPacman(const QStringList &packages) const;
    Command installAur(const QStringList &packages) const;
    Command installFlatpak(const QString &remote, const QStringList &appIds) const;
    Command updateFlatpak(const QStringList &appIds) const;
    Command uninstall(const QString &package, SearchItem::Repository type) const;

private:
    QString m_aurHelper;
};

#endif
