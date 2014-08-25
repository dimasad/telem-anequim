#include "MainWindow.h"
#include "TelemetryStream.h"

#include <QtGui>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QLabel *label = new QLabel(tr("0.0"));
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    
    setWindowTitle(tr("Telemetry"));
}