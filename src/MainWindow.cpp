#include "MainWindow.hpp"

#include <QtGui>
#include <QLabel>
#include <QVBoxLayout>


void
GaugeUpdater::update(const TelemetryVariable & var)
{
    for (auto gauge : m_gauges.values(var.label)) {
        gauge->setValue(var.value);
    }
}

void
GaugeUpdater::link(const QString &label, Gauge *gauge)
{
    m_gauges.insert(label, gauge);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QLabel *label = new QLabel("0.0");
    Gauge *gauge = new AngularGauge;
    gauge->setValue(20);
    m_updater.link("coolant temperature", gauge);
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(gauge);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    
    setWindowTitle(tr("Telemetry"));
}
