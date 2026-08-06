#include "engine/processrunner.h"
#include <QElapsedTimer>
#include <QProcess>
#include <QProcessEnvironment>

namespace {
// Intervalo entre as checagens de cancelamento/timeout.
constexpr int PollIntervalMs = 100;
constexpr int StartTimeoutMs = 5000;
constexpr int KillGraceMs    = 2000;
} // namespace

ProcessRunner::Result ProcessRunner::run(const QString &program,
                                         const QStringList &arguments,
                                         int timeoutMs,
                                         const QMap<QString, QString> &extraEnvironment) const
{
    Result result;
    const int generation = m_generation.loadAcquire();

    QProcess proc;
    if (!extraEnvironment.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (auto it = extraEnvironment.cbegin(); it != extraEnvironment.cend(); ++it)
            env.insert(it.key(), it.value());
        proc.setProcessEnvironment(env);
    }
    proc.start(program, arguments);

    if (!proc.waitForStarted(qMin(timeoutMs, StartTimeoutMs))) {
        // Binario ausente ou sem permissao: nao e "nenhum resultado".
        result.standardError = proc.errorString();
        return result;
    }
    result.started = true;

    QElapsedTimer timer;
    timer.start();

    while (!proc.waitForFinished(PollIntervalMs)) {
        // waitForFinished tambem devolve false quando o processo ja terminou.
        if (proc.state() == QProcess::NotRunning)
            break;
        if (m_generation.loadAcquire() != generation) {
            result.cancelled = true;
            break;
        }
        if (timer.hasExpired(timeoutMs)) {
            result.timedOut = true;
            break;
        }
    }

    if (result.cancelled || result.timedOut) {
        proc.kill();
        proc.waitForFinished(KillGraceMs);
    } else {
        result.exitCode = (proc.exitStatus() == QProcess::NormalExit)
            ? proc.exitCode()
            : -1;
    }

    result.standardOutput = QString::fromUtf8(proc.readAllStandardOutput());
    result.standardError  = QString::fromUtf8(proc.readAllStandardError());
    return result;
}

void ProcessRunner::cancelAll()
{
    m_generation.fetchAndAddOrdered(1);
}
