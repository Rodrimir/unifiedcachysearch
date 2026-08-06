#include "engine/commandrunner.h"
#include <QProcessEnvironment>

CommandRunner::CommandRunner(QObject *parent)
    : QObject(parent)
{
}

QProcess *CommandRunner::createProcess(const Command &command)
{
    auto *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);

    const auto &extraEnv = command.environment();
    if (!extraEnv.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (auto it = extraEnv.cbegin(); it != extraEnv.cend(); ++it)
            env.insert(it.key(), it.value());
        proc->setProcessEnvironment(env);
    }
    return proc;
}

void CommandRunner::reportFailedToStart(QProcess *process, const QString &label)
{
    // Com FailedToStart o sinal finished() nunca chega; sem tratar isso a
    // barra de progresso ficava girando para sempre.
    emit progressBarVisible(false);
    emit consoleMessage(QString("Não foi possível executar \"%1\": %2")
                            .arg(process->program(), process->errorString()), 2);
    emit commandFinished(false, label);
}

void CommandRunner::runCommand(const Command &command, const QString &actionLabel)
{
    if (command.isEmpty()) {
        emit commandFinished(false, actionLabel);
        return;
    }

    emit progressChanged(QString("%1: %2 ...").arg(actionLabel, command.toString()));
    emit progressBarVisible(true);

    auto *proc = createProcess(command);
    emit processAttached(proc);

    connect(proc, &QProcess::errorOccurred, this,
            [this, proc, actionLabel](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        reportFailedToStart(proc, actionLabel);
        proc->deleteLater();
    });

    connect(proc, &QProcess::finished, this,
            [this, proc, actionLabel](int exitCode, QProcess::ExitStatus status) {
        emit progressBarVisible(false);
        const bool ok = (status == QProcess::NormalExit && exitCode == 0);
        if (ok) {
            emit consoleMessage(QString("%1 concluída com sucesso.").arg(actionLabel), 1);
        } else {
            emit consoleMessage(QString("Falha na %1 (código de saída: %2).")
                                    .arg(actionLabel).arg(exitCode), 2);
        }
        emit commandFinished(ok, actionLabel);
        proc->deleteLater();
    });

    proc->start(command.program(), command.arguments());
}

void CommandRunner::runCommandsSequentially(const QList<Command> &commands,
                                            const QString &overallLabel)
{
    if (commands.isEmpty()) {
        emit progressBarVisible(false);
        emit allCommandsFinished(overallLabel);
        emit progressChanged(QString("%1 concluído.").arg(overallLabel));
        return;
    }

    const Command current = commands.first();
    const QList<Command> remaining = commands.mid(1);

    if (current.isEmpty()) {
        runCommandsSequentially(remaining, overallLabel);
        return;
    }

    emit progressChanged(QString("%1: %2 ...").arg(overallLabel, current.toString()));
    emit progressBarVisible(true);

    auto *proc = createProcess(current);
    emit processAttached(proc);

    connect(proc, &QProcess::errorOccurred, this,
            [this, proc, overallLabel](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        reportFailedToStart(proc, overallLabel);
        emit progressChanged(QString("Erro na %1.").arg(overallLabel));
        proc->deleteLater();
    });

    connect(proc, &QProcess::finished, this,
            [this, proc, remaining, overallLabel](int exitCode, QProcess::ExitStatus status) {
        proc->deleteLater();
        if (status == QProcess::NormalExit && exitCode == 0) {
            emit consoleMessage(
                QString("Comando concluído. %1 comando(s) restante(s).").arg(remaining.size()), 1);
            runCommandsSequentially(remaining, overallLabel);
        } else {
            emit consoleMessage(QString("Falha (código de saída: %1).").arg(exitCode), 2);
            emit progressChanged(QString("Erro na %1.").arg(overallLabel));
            emit progressBarVisible(false);
        }
    });

    proc->start(current.program(), current.arguments());
}
