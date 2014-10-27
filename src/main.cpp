#include "MainWindow.hpp"
#include <QApplication>

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(telemetryresources);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
