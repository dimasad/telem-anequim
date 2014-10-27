#include "Gauge.hpp"
#include "MainWindow.hpp"
#include "TelemetryStream.hpp"

#include <QtGui>
#include <QLabel>
#include <QVBoxLayout>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QLabel *label = new QLabel(tr("0.0"));
    Gauge *gauge = new Gauge;
    gauge->setValue(20);
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(gauge);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    
    setWindowTitle(tr("Telemetry"));
}
