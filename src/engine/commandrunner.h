#ifndef COMMANDRUNNER_H
#define COMMANDRUNNER_H

#include <QList>
#include <QObject>
#include <QProcess>
#include "core/command.h"

// Executa comandos externos de forma assincrona, um por vez ou em fila.
class CommandRunner : public QObject
{
    Q_OBJECT

public:
    explicit CommandRunner(QObject *parent = nullptr);

    void runCommand(const Command &command, const QString &actionLabel);
    void runCommandsSequentially(const QList<Command> &commands, const QString &overallLabel);

signals:
    void progressChanged(const QString &text);
    void progressBarVisible(bool visible);
    void commandFinished(bool success, const QString &action);
    void allCommandsFinished(const QString &overallLabel);
    void processAttached(QProcess *process);
    void consoleMessage(const QString &text, int type);

private:
    QProcess *createProcess(const Command &command);
    void reportFailedToStart(QProcess *process, const QString &label);
};

#endif
