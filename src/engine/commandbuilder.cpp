#include "engine/commandbuilder.h"
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>

namespace {

// Diretorio privado do usuario para o helper de senha. Nunca /tmp: la qualquer
// usuario da maquina poderia plantar o script antes e o sudo o executaria.
QString privateRuntimeDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty())
        return {};

    QDir().mkpath(dir);
    QFile::setPermissions(dir, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    return dir;
}

bool guiAskpassAvailable()
{
    if (!qEnvironmentVariableIsSet("DISPLAY")
        && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
        return false;
    return !QStandardPaths::findExecutable(QStringLiteral("kdialog")).isEmpty()
        || !QStandardPaths::findExecutable(QStringLiteral("zenity")).isEmpty();
}

// Script SUDO_ASKPASS: abre o dialogo de senha do desktop. Reescrito a cada
// execucao do app e legivel/executavel apenas pelo dono.
QString askpassScriptPath()
{
    static QMutex mutex;
    static QString cachedPath;
    static bool attempted = false;

    QMutexLocker lock(&mutex);
    if (attempted)
        return cachedPath;
    attempted = true;

    const QString dir = privateRuntimeDir();
    if (dir.isEmpty())
        return {};

    const QString path = dir + QStringLiteral("/cachysearch-askpass.sh");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return {};

    file.write(
        "#!/bin/sh\n"
        "if command -v kdialog >/dev/null 2>&1; then\n"
        "    exec kdialog --title \"CachySearch\" --password \"$*\"\n"
        "elif command -v zenity >/dev/null 2>&1; then\n"
        "    exec zenity --title \"CachySearch\" --password\n"
        "else\n"
        "    exit 1\n"
        "fi\n");
    file.close();

    if (!file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner))
        return {};

    cachedPath = path;
    return cachedPath;
}

} // namespace

CommandBuilder::CommandBuilder(const QString &aurHelper)
    : m_aurHelper(aurHelper)
{
}

Command CommandBuilder::installPacman(const QStringList &packages) const
{
    if (packages.isEmpty())
        return {};
    // -S e nao -Sy: sincronizar o banco e instalar um pacote isolado e um
    // partial upgrade, o cenario que quebra sistemas Arch/CachyOS.
    return Command(QStringLiteral("pkexec"),
                   QStringList{"pacman", "-S", "--noconfirm"} + packages);
}

Command CommandBuilder::installAur(const QStringList &packages) const
{
    if (packages.isEmpty() || m_aurHelper.isEmpty())
        return {};

    if (guiAskpassAvailable()) {
        const QString askpass = askpassScriptPath();
        if (!askpass.isEmpty()) {
            // sudo -A: dialogo grafico de senha, com cache de 5 min do sudo.
            return Command(m_aurHelper,
                           QStringList{"--sudoflags=-A", "-S", "--noconfirm"} + packages,
                           {{QStringLiteral("SUDO_ASKPASS"), askpass}});
        }
    }
    // Sem ambiente grafico: a senha e lida pelo stdin do console embutido.
    return Command(m_aurHelper,
                   QStringList{"--sudoflags=-S", "-S", "--noconfirm"} + packages);
}

Command CommandBuilder::installFlatpak(const QString &remote, const QStringList &appIds) const
{
    if (appIds.isEmpty())
        return {};
    return Command(QStringLiteral("flatpak"),
                   QStringList{"install", "-y", remote} + appIds);
}

Command CommandBuilder::updateFlatpak(const QStringList &appIds) const
{
    if (appIds.isEmpty())
        return {};
    return Command(QStringLiteral("flatpak"),
                   QStringList{"update", "-y"} + appIds);
}

Command CommandBuilder::uninstall(const QString &package, SearchItem::Repository type) const
{
    if (package.isEmpty())
        return {};

    switch (type) {
    case SearchItem::Pacman:
    case SearchItem::AUR:
        return Command(QStringLiteral("pkexec"),
                       {"pacman", "-Rns", "--noconfirm", package});
    case SearchItem::Flatpak:
        return Command(QStringLiteral("flatpak"),
                       {"uninstall", "-y", package});
    }
    return {};
}
