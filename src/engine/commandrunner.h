#ifndef COMMANDRUNNER_H
#define COMMANDRUNNER_H

#include <QObject>
#include <QStringList>
#include <QProcess>

class CommandRunner : public QObject
{
    Q_OBJECT

public:
    explicit CommandRunner(QObject *parent = nullptr);

    void runCommand(const QString &command, const QString &actionLabel);
    void runCommandsSequentially(const QStringList &commands, const QString &overallLabel);

signals:
    void progressChanged(const QString &text);
    void progressBarVisible(bool visible);
    void commandFinished(bool success, const QString &action);
    void allCommandsFinished(const QString &overallLabel);
    void processAttached(QProcess *process);
    void consoleMessage(const QString &text, int type);
};

#endif
