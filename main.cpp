#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application info
    app.setApplicationName("Package Tracker");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("MyCompany");
    
    MainWindow* mainWindow = new MainWindow();
    mainWindow->show();
    
    return app.exec();
}
