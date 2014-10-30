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
    AngularGauge *angularGauge = new AngularGauge;
    angularGauge->setNumMajorTicks(6);
    angularGauge->setValue(20);
    angularGauge->addLabel(u8"Temperature (\u00B0F)", 120, 200);
    angularGauge->setValueLabelPos(120, 220);    
    m_updater.link("coolant temperature", angularGauge);

    LinearGauge *linearGauge = new LinearGauge;
    linearGauge->setNumMajorTicks(6);
    linearGauge->setValue(25);
    m_updater.link("coolant temperature", linearGauge);
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(angularGauge);
    layout->addWidget(linearGauge);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    
    setWindowTitle(tr("Telemetry"));
}
