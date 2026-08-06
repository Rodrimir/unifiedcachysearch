#ifndef COMMAND_H
#define COMMAND_H

#include <QMap>
#include <QString>
#include <QStringList>

// Comando externo pronto para execucao.
//
// O programa, os argumentos e as variaveis de ambiente ficam separados desde a
// construcao ate o QProcess: nada passa por um shell e nada e reconstruido a
// partir de uma string. toString() existe apenas para exibir/exportar.
class Command
{
public:
    Command() = default;
    Command(QString program, QStringList arguments,
            QMap<QString, QString> environment = {});

    bool isEmpty() const { return m_program.isEmpty(); }

    const QString &program() const { return m_program; }
    const QStringList &arguments() const { return m_arguments; }
    const QMap<QString, QString> &environment() const { return m_environment; }

    // Representacao legivel (e colavel num shell) do comando.
    QString toString() const;

    bool operator==(const Command &other) const;
    bool operator!=(const Command &other) const { return !(*this == other); }

private:
    QString m_program;
    QStringList m_arguments;
    QMap<QString, QString> m_environment;
};

#endif
