#include <QApplication>
#include <QFile>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("NeiraBotPanel");
    app.setOrganizationName("Neira");

    // Load stylesheet
    QFile styleFile(":/style.qss");
    if (styleFile.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    MainWindow w;
    w.show();

    return app.exec();
}
