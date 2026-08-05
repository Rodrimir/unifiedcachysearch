#include "engine/commandrunner.h"

CommandRunner::CommandRunner(QObject *parent)
    : QObject(parent)
{
}

void CommandRunner::runCommand(const QString &command, const QString &actionLabel)
{
    emit progressChanged(QString("%1: %2 ...").arg(actionLabel, command));
    emit progressBarVisible(true);

    QStringList parts = command.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        emit commandFinished(false, actionLabel);
        return;
    }

    QString program = parts.first();
    QStringList args = parts.mid(1);

    auto *proc = new QProcess(this);
    emit processAttached(proc);

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, actionLabel](int exitCode, QProcess::ExitStatus status) {
        emit progressBarVisible(false);
        bool ok = (status == QProcess::NormalExit && exitCode == 0);
        if (ok) {
            emit consoleMessage(QString("%1 concluída com sucesso.").arg(actionLabel), 1);
        } else {
            QString err = QString::fromUtf8(proc->readAllStandardError());
            emit consoleMessage(QString("Falha na %1.\n%2").arg(actionLabel, err), 2);
        }
        emit commandFinished(ok, actionLabel);
        proc->deleteLater();
    });

    proc->setProcessChannelMode(QProcess::MergedChannels);
    proc->start(program, args);
}

void CommandRunner::runCommandsSequentially(const QStringList &commands,
                                             const QString &overallLabel)
{
    if (commands.isEmpty()) {
        emit progressBarVisible(false);
        emit allCommandsFinished(overallLabel);
        emit progressChanged(QString("%1 concluído.").arg(overallLabel));
        return;
    }

    emit progressChanged(QString("%1: %2 ...").arg(overallLabel, commands.first()));
    emit progressBarVisible(true);

    QStringList parts = commands.first().split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        runCommandsSequentially(commands.mid(1), overallLabel);
        return;
    }

    QStringList remaining = commands.mid(1);
    auto *proc = new QProcess(this);
    emit processAttached(proc);

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, remaining, overallLabel](int exitCode, QProcess::ExitStatus status) {
        proc->deleteLater();
        if (status == QProcess::NormalExit && exitCode == 0) {
            emit consoleMessage(
                QString("Comando concluído. %1 comando(s) restante(s).").arg(remaining.size()), 1);
            runCommandsSequentially(remaining, overallLabel);
        } else {
            QString err = QString::fromUtf8(proc->readAllStandardError());
            emit consoleMessage(QString("Falha.\n%1").arg(err), 2);
            emit progressChanged(QString("Erro na %1.").arg(overallLabel));
            emit progressBarVisible(false);
        }
    });

    proc->setProcessChannelMode(QProcess::MergedChannels);
    proc->start(parts.first(), parts.mid(1));
}
