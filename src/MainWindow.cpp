#include "MainWindow.hpp"

#include <QtGui>
#include <QAction>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSerialPortInfo>
#include <QToolBar>
#include <QVBoxLayout>

#include <QDebug>

void
GaugeUpdater::update(const TelemetryVariable &var)
{
    for (auto &updater: m_updaters.values(var.label)) {
        updater(var.value);
    }
}


void
GaugeUpdater::link(const QString &label, updater updater)
{
    m_updaters.insertMulti(label, updater);
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
    m_efisStream = new EfisStream(m_settings.efisPort(), this);
    m_emsStream = new EmsStream(m_settings.emsPort(), this);
    
    connect(&m_settings, SIGNAL(efisPortChanged(const QString &)),
            m_efisStream, SLOT(setPort(const QString &)));
    connect(&m_settings, SIGNAL(emsPortChanged(const QString &)),
            m_emsStream, SLOT(setPort(const QString &)));
    connect(m_efisStream, SIGNAL(variableUpdated(const TelemetryVariable &)),
            &m_updater, SLOT(update(const TelemetryVariable &)));
    connect(m_emsStream, SIGNAL(variableUpdated(const TelemetryVariable &)),
            &m_updater, SLOT(update(const TelemetryVariable &)));
    
    auto showSettings = new QAction("Settings", this);
    showSettings->setIcon(QIcon(":/images/settings.ico"));
    connect(showSettings, SIGNAL(triggered()), 
            this, SLOT(showSettingsDialog()));
    
    auto mainToolBar = addToolBar("Main");
    mainToolBar->setMovable(false);
    mainToolBar->addAction(showSettings);
    
    auto rpmGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    rpmGauge->setValueRange(0, 200);
    rpmGauge->setAngleRange(-90, 90);
    rpmGauge->setNumMajorTicks(5);
    rpmGauge->setValueLabelPos(120, 220);
    rpmGauge->setTextColor(QColor("white"));
    m_updater.link("RPM", [=](double value){rpmGauge->setValue(value);});
    
    auto cht1Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht1Gauge->setValueRange(0, 500);
    cht1Gauge->setNumMajorTicks(3);
    m_updater.link("cht1", [=](double value){cht1Gauge->setValue(value);});

    auto cht2Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht2Gauge->setValueRange(0, 500);
    cht2Gauge->setNumMajorTicks(3);
    m_updater.link("cht2", [=](double value){cht2Gauge->setValue(value);});

    auto cht3Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht3Gauge->setValueRange(0, 500);
    cht3Gauge->setNumMajorTicks(3);
    m_updater.link("cht3", [=](double value){cht3Gauge->setValue(value);});

    auto cht4Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht4Gauge->setValueRange(0, 500);
    cht4Gauge->setNumMajorTicks(3);
    m_updater.link("cht4", [=](double value){cht4Gauge->setValue(value);});
    
    auto chtLayout = new QVBoxLayout;
    chtLayout->setSpacing(0);
    chtLayout->addWidget(cht1Gauge);
    chtLayout->addWidget(cht2Gauge);
    chtLayout->addWidget(cht3Gauge);
    chtLayout->addWidget(cht4Gauge);
    
    auto chtGroupBox = new QGroupBox("CHT");
    chtGroupBox->setAlignment(Qt::AlignHCenter);
    chtGroupBox->setLayout(chtLayout);

    auto oilPressGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    oilPressGauge->setValueRange(0, 200);
    oilPressGauge->setAngleRange(-90, 90);
    oilPressGauge->setNumMajorTicks(5);
    oilPressGauge->setValueLabelPos(120, 220);
    oilPressGauge->setTextColor(QColor("white"));
    m_updater.link("oil pressure",
                   [=](double value){oilPressGauge->setValue(value);});
 
    auto column1Layout = new QVBoxLayout;
    column1Layout->addWidget(rpmGauge);
    column1Layout->addWidget(chtGroupBox);
    column1Layout->addWidget(oilPressGauge);
 
    auto mapGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    mapGauge->setValueRange(0, 200);
    mapGauge->setAngleRange(-90, 90);
    mapGauge->setNumMajorTicks(5);
    mapGauge->setValueLabelPos(120, 220);
    mapGauge->setTextColor(QColor("white"));
    m_updater.link("manifold pressure",
                   [=](double value){mapGauge->setValue(value);});
    
    auto egt1Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt1Gauge->setValueRange(0, 500);
    egt1Gauge->setNumMajorTicks(3);
    m_updater.link("egt1", [=](double value){egt1Gauge->setValue(value);});

    auto egt2Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt2Gauge->setValueRange(0, 500);
    egt2Gauge->setNumMajorTicks(3);
    m_updater.link("egt2", [=](double value){egt2Gauge->setValue(value);});

    auto egt3Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt3Gauge->setValueRange(0, 500);
    egt3Gauge->setNumMajorTicks(3);
    m_updater.link("egt3", [=](double value){egt3Gauge->setValue(value);});

    auto egt4Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt4Gauge->setValueRange(0, 500);
    egt4Gauge->setNumMajorTicks(3);
    m_updater.link("egt4", [=](double value){egt4Gauge->setValue(value);});
    
    auto egtLayout = new QVBoxLayout;
    egtLayout->setSpacing(0);
    egtLayout->addWidget(egt1Gauge);
    egtLayout->addWidget(egt2Gauge);
    egtLayout->addWidget(egt3Gauge);
    egtLayout->addWidget(egt4Gauge);
    
    auto egtGroupBox = new QGroupBox("EGT");
    egtGroupBox->setAlignment(Qt::AlignHCenter);
    egtGroupBox->setLayout(egtLayout);

    auto oilTempGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    oilTempGauge->setValueRange(0, 200);
    oilTempGauge->setAngleRange(-90, 90);
    oilTempGauge->setNumMajorTicks(5);
    oilTempGauge->setValueLabelPos(120, 220);
    oilTempGauge->setTextColor(QColor("white"));
    m_updater.link("oil temperature",
                   [=](double value){oilTempGauge->setValue(value);});
    
    auto column2Layout = new QVBoxLayout;
    column2Layout->addWidget(mapGauge);
    column2Layout->addWidget(egtGroupBox);
    column2Layout->addWidget(oilTempGauge);

    auto fuelPressGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelPressGauge->setValueRange(0, 200);
    fuelPressGauge->setAngleRange(-90, 90);
    fuelPressGauge->setNumMajorTicks(5);
    fuelPressGauge->setValueLabelPos(120, 220);
    fuelPressGauge->setTextColor(QColor("white"));
    m_updater.link("fuel pressure",
                   [=](double value){fuelPressGauge->setValue(value);});

    auto fuelLevel1Gauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelLevel1Gauge->setValueRange(0, 200);
    fuelLevel1Gauge->setAngleRange(-90, 90);
    fuelLevel1Gauge->setNumMajorTicks(5);
    fuelLevel1Gauge->setValueLabelPos(120, 220);
    fuelLevel1Gauge->setTextColor(QColor("white"));
    m_updater.link("fuel level 1",
                   [=](double value){fuelLevel1Gauge->setValue(value);});


    auto fuelLevel2Gauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelLevel2Gauge->setValueRange(0, 200);
    fuelLevel2Gauge->setAngleRange(-90, 90);
    fuelLevel2Gauge->setNumMajorTicks(5);
    fuelLevel2Gauge->setValueLabelPos(120, 220);
    fuelLevel2Gauge->setTextColor(QColor("white"));
    m_updater.link("fuel level 2",
                   [=](double value){fuelLevel2Gauge->setValue(value);});
    
    auto column3Layout = new QVBoxLayout;
    column3Layout->addWidget(fuelPressGauge);
    column3Layout->addWidget(fuelLevel1Gauge);
    column3Layout->addWidget(fuelLevel2Gauge);
    
    auto mainLayout = new QHBoxLayout;
    mainLayout->addItem(column1Layout);
    mainLayout->addItem(column2Layout);
    mainLayout->addItem(column3Layout);
    
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    
    setWindowTitle(tr("Telemetry"));
}


void
MainWindow::showSettingsDialog()
{
    SettingsDialog settingsDialog(&m_settings);
    settingsDialog.exec();
}
