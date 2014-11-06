#include "MainWindow.hpp"

#include <QtGui>
#include <QAction>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSerialPortInfo>
#include <QToolBar>
#include <QVBoxLayout>

#include <QDebug>

void
GaugeUpdater::update(const TelemetryVariable &var)
{
    for (auto gauge : m_gauges.values(var.label)) {
        gauge->setValue(var.value);
    }
}


void
GaugeUpdater::link(const QString &label, Gauge *gauge)
{
    m_gauges.insertMulti(label, gauge);
}


Settings::Settings() :
    m_storedSettings("CEA", "telem-anequim")
{
    m_emsPort = m_storedSettings.value("ems_port").toString();
    m_efisPort = m_storedSettings.value("efis_port").toString();
}


void
Settings::setEmsPort(const QString &newEmsPort)
{
    m_emsPort = newEmsPort;
    m_storedSettings.setValue("ems_port", newEmsPort);

    emit emsPortChanged(newEmsPort);
}


void
Settings::setEfisPort(const QString &newEfisPort)
{
    m_efisPort = newEfisPort;
    m_storedSettings.setValue("efis_port", newEfisPort);

    emit efisPortChanged(newEfisPort);
}


void 
Settings::sync()
{
    m_storedSettings.sync();
}


SettingsDialog::SettingsDialog(Settings *settings, QWidget *parent) :
    QDialog(parent), m_settings(settings)
{
    QStringList portNames;
    portNames.append("");
    for (const auto &portInfo : QSerialPortInfo::availablePorts())
        portNames.append(portInfo.portName());
    
    m_efisPortComboBox.addItems(portNames);
    m_emsPortComboBox.addItems(portNames);
    setCurrentPort(settings->emsPort(), m_emsPortComboBox);
    setCurrentPort(settings->efisPort(), m_efisPortComboBox);
    
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                          | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    auto layout = new QFormLayout;
    layout->addRow("EFIS port:", &m_efisPortComboBox);
    layout->addRow("EMS port:", &m_emsPortComboBox);
    layout->addRow(buttonBox);
    setLayout(layout);
    
    connect(this, SIGNAL(accepted()), this, SLOT(saveSettings()));
}


void
SettingsDialog::saveSettings()
{
    m_settings->setEfisPort(m_efisPortComboBox.currentText());
    m_settings->setEmsPort(m_emsPortComboBox.currentText());
    m_settings->sync();
}


void
SettingsDialog::setCurrentPort(const QString &portName, QComboBox &comboBox)
{
    int index = comboBox.findText(portName);
    if (index == -1) {
        comboBox.insertItem(0, portName);
        index = 0;
    }
    comboBox.setCurrentIndex(index);
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    auto showSettings = new QAction("Settings", this);
    showSettings->setIcon(QIcon(":/images/settings.ico"));
    connect(showSettings, SIGNAL(triggered()), 
            this, SLOT(showSettingsDialog()));

    auto mainToolBar = addToolBar("Main");
    mainToolBar->setMovable(false);
    mainToolBar->addAction(showSettings);
    
    auto *angularGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    angularGauge->setValueRange(0, 200);
    angularGauge->setAngleRange(-90, 90);
    angularGauge->setNumMajorTicks(5);
    angularGauge->setValueLabelPos(120, 220);
    angularGauge->setTextColor(QColor("white"));
    angularGauge->setValue(30);
    //angularGauge->addLabel(u8"Oil temperature (\u00B0F)", 120, 200);
    //m_updater.link("oil temperature", angularGauge);

    auto *cht1Gauge = new HorizontalLinearGauge;
    cht1Gauge->setValueLimits(0, 500);
    cht1Gauge->setNumMajorTicks(6);
    m_updater.link("cht1", cht1Gauge);

    auto *cht2Gauge = new HorizontalLinearGauge;
    cht2Gauge->setValueLimits(0, 500);
    cht2Gauge->setNumMajorTicks(6);
    m_updater.link("cht2", cht2Gauge);
    
    auto *cht3Gauge = new HorizontalLinearGauge;
    cht3Gauge->setValueLimits(0, 500);
    cht3Gauge->setNumMajorTicks(6);
    m_updater.link("cht3", cht3Gauge);

    auto *cht4Gauge = new HorizontalLinearGauge;
    cht4Gauge->setValueLimits(0, 500);
    cht4Gauge->setNumMajorTicks(6);
    m_updater.link("cht4", cht4Gauge);
    
    auto *chtLayout = new QVBoxLayout;
    chtLayout->addWidget(cht1Gauge);
    chtLayout->addWidget(cht2Gauge);
    chtLayout->addWidget(cht3Gauge);
    chtLayout->addWidget(cht4Gauge);
    
    auto *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(angularGauge);
    mainLayout->addItem(chtLayout);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    
    setWindowTitle(tr("Telemetry"));
    
    /*
    m_emsStream = new EmsStream("/dev/ttyUSB0");
    connect(m_emsStream, SIGNAL(variableUpdated(const TelemetryVariable &)),
            &m_updater, SLOT(update(const TelemetryVariable &)));
    */
}


void
MainWindow::showSettingsDialog()
{
    SettingsDialog settingsDialog(&m_settings);
    settingsDialog.exec();
}
