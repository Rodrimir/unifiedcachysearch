#include "core/exportservice.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

bool ExportService::exportResults(const QList<SearchItem> &items,
                                  const QString &filePath,
                                  QString *errorOut)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorOut)
            *errorOut = "Não foi possível abrir o arquivo para escrita.";
        return false;
    }

    QTextStream out(&file);

    if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
        QJsonArray arr;
        for (const auto &item : items) {
            QJsonObject obj;
            obj["name"] = item.name();
            obj["version"] = item.version();
            obj["repository"] = item.repository();
            obj["displayName"] = item.displayName();
            obj["description"] = item.description();
            obj["installCommand"] = item.installCommand();
            obj["isInstalled"] = item.isInstalled();

            QString repoType;
            switch (item.repositoryType()) {
            case SearchItem::Pacman:  repoType = "Pacman"; break;
            case SearchItem::AUR:     repoType = "AUR"; break;
            case SearchItem::Flatpak: repoType = "Flatpak"; break;
            }
            obj["repositoryType"] = repoType;

            arr.append(obj);
        }
        QJsonDocument doc(arr);
        file.write(doc.toJson(QJsonDocument::Indented));
    } else if (filePath.endsWith(".csv", Qt::CaseInsensitive)) {
        out << "Nome,Versão,Repositório,Tipo,Descrição,Comando,Instalado\n";
        for (const auto &item : items) {
            QString repoType;
            switch (item.repositoryType()) {
            case SearchItem::Pacman:  repoType = "Pacman"; break;
            case SearchItem::AUR:     repoType = "AUR"; break;
            case SearchItem::Flatpak: repoType = "Flatpak"; break;
            }
            out << "\"" << item.name() << "\","
                << "\"" << item.version() << "\","
                << "\"" << item.repository() << "\","
                << "\"" << repoType << "\","
                << "\"" << item.description() << "\","
                << "\"" << item.installCommand() << "\","
                << (item.isInstalled() ? "Sim" : "Não") << "\n";
        }
    } else {
        out << "=== CachyOS Unified Search - Resultados ===\n"
            << "Data: " << QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss") << "\n"
            << "Total: " << items.size() << " pacotes\n\n";
        for (const auto &item : items) {
            out << "[" << item.repository() << "] "
                << item.name() << " (" << item.version() << ")\n";
            if (!item.description().isEmpty())
                out << "    " << item.description() << "\n";
            out << "    Comando: " << item.installCommand() << "\n";
            out << "    Status: " << (item.isInstalled() ? "Instalado" : "Não instalado") << "\n\n";
        }
    }

    file.close();
    return true;
}
