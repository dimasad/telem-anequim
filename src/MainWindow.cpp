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
    Gauge *gauge = new AngularGauge;
    gauge->setNumMajorTicks(6);
    gauge->setValue(20);
    gauge->addLabel(u8"Temperature (\u00B0F)", 120, 200);
    gauge->setValueLabelPos(120, 220);    
    m_updater.link("coolant temperature", gauge);
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(gauge);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    
    setWindowTitle(tr("Telemetry"));
}
