#include <QApplication>
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("CachyOS Unified Search");
    app.setOrganizationName("CachyOS");
    app.setApplicationVersion(APP_VERSION);

    MainWindow w;
    w.show();
    return app.exec();
}
