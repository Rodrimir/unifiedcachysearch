#ifndef PROCESSRUNNER_H
#define PROCESSRUNNER_H

#include <QAtomicInt>
#include <QMap>
#include <QString>
#include <QStringList>

// Execucao sincrona de processos auxiliares (pacman -Ss, flatpak search, ...),
// feita para rodar dentro de tarefas do QtConcurrent.
//
// O cancelamento nao toca no QProcess de outra thread: cancelAll() apenas
// incrementa um contador e cada run() em andamento percebe a mudanca e mata o
// proprio processo. Assim nenhum QProcess e compartilhado entre threads.
class ProcessRunner
{
public:
    struct Result {
        int exitCode = -1;
        QString standardOutput;
        QString standardError;
        bool started = false;
        bool timedOut = false;
        bool cancelled = false;

        bool succeeded() const
        {
            return started && !timedOut && !cancelled && exitCode == 0;
        }
    };

    // extraEnvironment serve para forcar LC_ALL=C onde o parsing depende dos
    // rotulos em ingles da saida.
    Result run(const QString &program, const QStringList &arguments, int timeoutMs,
               const QMap<QString, QString> &extraEnvironment = {}) const;

    // Aborta tudo que estiver em execucao. Pode ser chamado de qualquer thread.
    void cancelAll();

private:
    QAtomicInt m_generation;
};

#endif
