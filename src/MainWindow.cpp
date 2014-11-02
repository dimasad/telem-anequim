#include "MainWindow.hpp"

#include <QtGui>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSerialPortInfo>
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
    m_gauges.insert(label, gauge);
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
    SettingsDialog settingsDialog(&m_settings);
    settingsDialog.exec();

    AngularGauge *angularGauge = new AngularGauge;
    angularGauge->setNumMajorTicks(6);
    angularGauge->setValue(20);
    angularGauge->addLabel(u8"Temperature (\u00B0F)", 120, 200);
    angularGauge->setValueLabelPos(120, 220);    
    m_updater.link("coolant temperature", angularGauge);

    HorizontalLinearGauge *horizontalLinearGauge = new HorizontalLinearGauge;
    horizontalLinearGauge->setNumMajorTicks(6);
    horizontalLinearGauge->setValue(25);
    m_updater.link("coolant temperature", horizontalLinearGauge);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(angularGauge);
    layout->addWidget(horizontalLinearGauge);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    
    setWindowTitle(tr("Telemetry"));
}
