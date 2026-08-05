#ifndef EXPORTSERVICE_H
#define EXPORTSERVICE_H

#include <QString>
#include <QList>
#include "core/searchitem.h"

class ExportService
{
public:
    static bool exportResults(const QList<SearchItem> &items,
                              const QString &filePath,
                              QString *errorOut = nullptr);
};

#endif
