#include "core/exportservice.h"
#include "core/repositoryinfo.h"
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace {

// Campo CSV: aspas internas viram aspas duplicadas (RFC 4180). Sem isso uma
// descricao com " corrompia a linha inteira.
QString csvField(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('"'), QLatin1String("\"\""));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}

bool writeJson(QFile &file, const QList<SearchItem> &items)
{
    QJsonArray arr;
    for (const auto &item : items) {
        QJsonObject obj;
        obj["name"] = item.name();
        obj["version"] = item.version();
        obj["repository"] = item.repository();
        obj["displayName"] = item.displayName();
        obj["description"] = item.description();
        obj["installCommand"] = item.installCommandText();
        obj["isInstalled"] = item.isInstalled();
        obj["repositoryType"] = RepositoryInfo::name(item.repositoryType());
        arr.append(obj);
    }
    return file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented)) >= 0;
}

void writeCsv(QTextStream &out, const QList<SearchItem> &items)
{
    out << "Nome,Versão,Repositório,Tipo,Descrição,Comando,Instalado\n";
    for (const auto &item : items) {
        out << csvField(item.name()) << ','
            << csvField(item.version()) << ','
            << csvField(item.repository()) << ','
            << csvField(RepositoryInfo::name(item.repositoryType())) << ','
            << csvField(item.description()) << ','
            << csvField(item.installCommandText()) << ','
            << (item.isInstalled() ? "Sim" : "Não") << '\n';
    }
}

void writeText(QTextStream &out, const QList<SearchItem> &items)
{
    out << "=== CachyOS Unified Search - Resultados ===\n"
        << "Data: " << QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss") << "\n"
        << "Total: " << items.size() << " pacotes\n\n";
    for (const auto &item : items) {
        out << "[" << item.repository() << "] "
            << item.name() << " (" << item.version() << ")\n";
        if (!item.description().isEmpty())
            out << "    " << item.description() << "\n";
        out << "    Comando: " << item.installCommandText() << "\n"
            << "    Status: " << (item.isInstalled() ? "Instalado" : "Não instalado") << "\n\n";
    }
}

} // namespace

bool ExportService::exportResults(const QList<SearchItem> &items,
                                  const QString &filePath,
                                  QString *errorOut)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorOut)
            *errorOut = QStringLiteral("Não foi possível abrir o arquivo para escrita.");
        return false;
    }

    bool ok = true;
    if (filePath.endsWith(".json", Qt::CaseInsensitive)) {
        ok = writeJson(file, items);
    } else {
        QTextStream out(&file);
        if (filePath.endsWith(".csv", Qt::CaseInsensitive))
            writeCsv(out, items);
        else
            writeText(out, items);
        // Sem o flush explicito o buffer do QTextStream se perdia no close()
        // abaixo e o arquivo saia truncado.
        out.flush();
        ok = (out.status() == QTextStream::Ok);
    }

    if (file.error() != QFile::NoError)
        ok = false;
    file.close();

    if (!ok && errorOut)
        *errorOut = QStringLiteral("Falha ao gravar o arquivo (disco cheio ou sem permissão?).");
    return ok;
}
