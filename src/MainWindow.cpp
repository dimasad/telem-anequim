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
    rpmGauge->setValueRange(0, 3500);
    rpmGauge->setAngleRange(-135, 90);
    rpmGauge->setNumMajorTicks(8);
    rpmGauge->setValueLabelPos(152, 220);
    rpmGauge->setTextColor(QColor("white"));
    rpmGauge->addRangeBand(QColor("darkgreen"), 600, 2700);
    rpmGauge->addRangeBand(QColor("yellow"), 2700, 3200);
    rpmGauge->addRangeBand(QColor("darkred"), 3200, 3500);
    rpmGauge->addLabel("RPM", 152, 200);
    m_updater.link("RPM", [=](double value){rpmGauge->setValue(value);});
    
    auto cht1Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht1Gauge->setValueRange(150, 500);
    cht1Gauge->setNumMajorTicks(8);
    cht1Gauge->setTextColor(QColor("white"));
    cht1Gauge->addRangeBand(QColor("darkred"), 0, 150);
    cht1Gauge->addRangeBand(QColor("yellow"), 150, 200);
    cht1Gauge->addRangeBand(QColor("darkgreen"), 200, 435);
    cht1Gauge->addRangeBand(QColor("yellow"), 435, 450);
    cht1Gauge->addRangeBand(QColor("darkred"), 450, 500);
    m_updater.link("cht1", [=](double value){cht1Gauge->setValue(value);});

    auto cht2Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht2Gauge->setValueRange(150, 500);
    cht2Gauge->setNumMajorTicks(8);
    cht2Gauge->setTextColor(QColor("white"));
    cht2Gauge->addRangeBand(QColor("darkred"), 0, 150);
    cht2Gauge->addRangeBand(QColor("yellow"), 150, 200);
    cht2Gauge->addRangeBand(QColor("darkgreen"), 200, 435);
    cht2Gauge->addRangeBand(QColor("yellow"), 435, 450);
    cht2Gauge->addRangeBand(QColor("darkred"), 450, 500);
    m_updater.link("cht2", [=](double value){cht2Gauge->setValue(value);});

    auto cht3Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht3Gauge->setValueRange(150, 500);
    cht3Gauge->setNumMajorTicks(8);
    cht3Gauge->setTextColor(QColor("white"));
    cht3Gauge->addRangeBand(QColor("darkred"), 0, 150);
    cht3Gauge->addRangeBand(QColor("yellow"), 150, 200);
    cht3Gauge->addRangeBand(QColor("darkgreen"), 200, 435);
    cht3Gauge->addRangeBand(QColor("yellow"), 435, 450);
    cht3Gauge->addRangeBand(QColor("darkred"), 450, 500);
    m_updater.link("cht3", [=](double value){cht3Gauge->setValue(value);});

    auto cht4Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht4Gauge->setValueRange(150, 500);
    cht4Gauge->setNumMajorTicks(8);
    cht4Gauge->setTextColor(QColor("white"));
    cht4Gauge->addRangeBand(QColor("darkred"), 0, 150);
    cht4Gauge->addRangeBand(QColor("yellow"), 150, 200);
    cht4Gauge->addRangeBand(QColor("darkgreen"), 200, 435);
    cht4Gauge->addRangeBand(QColor("yellow"), 435, 450);
    cht4Gauge->addRangeBand(QColor("darkred"), 450, 500);
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
    oilPressGauge->setValueRange(0, 100);
    oilPressGauge->setAngleRange(-135, 90);
    oilPressGauge->setNumMajorTicks(11);
    oilPressGauge->addRangeBand(QColor("darkred"), 0, 15);
    oilPressGauge->addRangeBand(QColor("yellow"), 15, 20);
    oilPressGauge->addRangeBand(QColor("darkgreen"), 20, 90);
    oilPressGauge->addRangeBand(QColor("yellow"), 90, 95);
    oilPressGauge->addRangeBand(QColor("darkred"), 95, 100);
    oilPressGauge->setValueLabelPos(152, 220);
    oilPressGauge->setTextColor(QColor("white"));
    oilPressGauge->addLabel("Oil pressure (psi)", 152, 200);
    m_updater.link("oil pressure",
                   [=](double value){oilPressGauge->setValue(value);});
 
    auto mapGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    mapGauge->setValueRange(0, 40);
    mapGauge->setAngleRange(-135, 90);
    mapGauge->setNumMajorTicks(11);
    mapGauge->addRangeBand(QColor("darkgreen"), 0, 36);
    mapGauge->addRangeBand(QColor("yellow"), 36, 38);
    mapGauge->addRangeBand(QColor("darkred"), 38, 40);
    mapGauge->setValueLabelPos(152, 220);
    mapGauge->setTextColor(QColor("white"));
    mapGauge->addLabel("MAP (inHg)", 152, 200);
    m_updater.link("manifold pressure",
                   [=](double value){mapGauge->setValue(value);});
    
    auto egt1Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt1Gauge->setValueRange(800, 1600);
    egt1Gauge->setNumMajorTicks(6);
    egt1Gauge->setTextColor(QColor("white"));
    egt1Gauge->addRangeBand(QColor("darkgreen"), 400, 1500);
    egt1Gauge->addRangeBand(QColor("yellow"), 1500, 1600);
    m_updater.link("egt1", [=](double value){egt1Gauge->setValue(value);});

    auto egt2Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt2Gauge->setValueRange(800, 1600);
    egt2Gauge->setNumMajorTicks(6);
    egt2Gauge->setTextColor(QColor("white"));
    egt2Gauge->addRangeBand(QColor("darkgreen"), 400, 1500);
    egt2Gauge->addRangeBand(QColor("yellow"), 1500, 1600);
    m_updater.link("egt2", [=](double value){egt2Gauge->setValue(value);});

    auto egt3Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt3Gauge->setValueRange(800, 1600);
    egt3Gauge->setNumMajorTicks(6);
    egt3Gauge->setTextColor(QColor("white"));
    egt3Gauge->addRangeBand(QColor("darkgreen"), 400, 1500);
    egt3Gauge->addRangeBand(QColor("yellow"), 1500, 1600);
    m_updater.link("egt3", [=](double value){egt3Gauge->setValue(value);});

    auto egt4Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt4Gauge->setValueRange(800, 1600);
    egt4Gauge->setNumMajorTicks(6);
    egt4Gauge->setTextColor(QColor("white"));
    egt4Gauge->addRangeBand(QColor("darkgreen"), 400, 1500);
    egt4Gauge->addRangeBand(QColor("yellow"), 1500, 1600);
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
    oilTempGauge->setValueRange(60, 260);
    oilTempGauge->setAngleRange(-135, 90);
    oilTempGauge->setNumMajorTicks(11);
    oilTempGauge->addRangeBand(QColor("yellow"), 60, 100);
    oilTempGauge->addRangeBand(QColor("darkgreen"), 100, 220);
    oilTempGauge->addRangeBand(QColor("yellow"), 220, 240);
    oilTempGauge->addRangeBand(QColor("darkred"), 240, 260);
    oilTempGauge->setValueLabelPos(152, 220);
    oilTempGauge->setTextColor(QColor("white"));
    oilTempGauge->addLabel("Oil temperature (\u00B0F)", 152, 200);
    m_updater.link("oil temperature",
                   [=](double value){oilTempGauge->setValue(value);});
    
    auto fuelPressGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelPressGauge->setValueRange(0, 30);
    fuelPressGauge->setAngleRange(-135, 90);
    fuelPressGauge->setNumMajorTicks(7);
    fuelPressGauge->addRangeBand(QColor("darkred"), 0, 10);
    fuelPressGauge->addRangeBand(QColor("yellow"), 10, 15);
    fuelPressGauge->addRangeBand(QColor("darkgreen"), 15, 27);
    fuelPressGauge->addRangeBand(QColor("yellow"), 27, 28);
    fuelPressGauge->addRangeBand(QColor("darkred"), 28, 30);
    fuelPressGauge->setValueLabelPos(152, 220);
    fuelPressGauge->setTextColor(QColor("white"));
    fuelPressGauge->addLabel("Fuel pressure (psi)", 152, 200);
    m_updater.link("fuel pressure",
                   [=](double value){fuelPressGauge->setValue(value);});

    auto fuelLevel1Gauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelLevel1Gauge->setValueRange(0, 24);
    fuelLevel1Gauge->setAngleRange(-135, 90);
    fuelLevel1Gauge->setNumMajorTicks(7);
    fuelLevel1Gauge->addRangeBand(QColor("darkred"), 0, 1.3);
    fuelLevel1Gauge->addRangeBand(QColor("yellow"), 1.3, 1.5);
    fuelLevel1Gauge->addRangeBand(QColor("darkgreen"), 1.5, 24);
    fuelLevel1Gauge->setValueLabelPos(152, 220);
    fuelLevel1Gauge->setTextColor(QColor("white"));
    fuelLevel1Gauge->addLabel("Fuel level 1 (gal)", 152, 200);
    m_updater.link("fuel level 1",
                   [=](double value){fuelLevel1Gauge->setValue(value);});


    auto fuelLevel2Gauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelLevel2Gauge->setValueRange(0, 4);
    fuelLevel2Gauge->setAngleRange(-135, 90);
    fuelLevel2Gauge->setNumMajorTicks(6);
    fuelLevel2Gauge->addRangeBand(QColor("darkred"), 0, 1.3);
    fuelLevel2Gauge->addRangeBand(QColor("yellow"), 1.3, 1.5);
    fuelLevel2Gauge->addRangeBand(QColor("darkgreen"), 1.5, 4);
    fuelLevel2Gauge->setValueLabelPos(152, 220);
    fuelLevel2Gauge->setTextColor(QColor("white"));
    fuelLevel2Gauge->addLabel("Fuel level 2 (gal)", 152, 200);
    m_updater.link("fuel level 2",
                   [=](double value){fuelLevel2Gauge->setValue(value);});

    auto fuelFlowGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelFlowGauge->setValueRange(0, 25);
    fuelFlowGauge->setAngleRange(-135, 90);
    fuelFlowGauge->setNumMajorTicks(5);
    fuelFlowGauge->addRangeBand(QColor("darkgreen"), 0, 22);
    fuelFlowGauge->addRangeBand(QColor("yellow"), 22, 25);
    fuelFlowGauge->setValueLabelPos(152, 220);
    fuelFlowGauge->setTextColor(QColor("white"));
    fuelFlowGauge->addLabel("Fuel flow (gal/h)", 152, 200);
    m_updater.link("fuel flow",
                   [=](double value){fuelFlowGauge->setValue(value);});

    auto lambdaGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    lambdaGauge->setValueRange(0, 100);
    lambdaGauge->setAngleRange(-135, 90);
    lambdaGauge->setNumMajorTicks(11);
    lambdaGauge->addRangeBand(QColor("darkgreen"), 30, 35);
    lambdaGauge->setValueLabelPos(152, 220);
    lambdaGauge->setTextColor(QColor("white"));
    lambdaGauge->addLabel("Lambda", 152, 200);
    m_updater.link("aileron trim",
                   [=](double value){lambdaGauge->setValue(value);});
    
    auto altitudeGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    altitudeGauge->setValueRange(0, 10000);
    altitudeGauge->setAngleRange(-135, 90);
    altitudeGauge->setNumMajorTicks(11);
    altitudeGauge->setValueLabelPos(152, 220);
    altitudeGauge->setTextColor(QColor("white"));
    altitudeGauge->addLabel("Altitude (ft)", 152, 200);
    m_updater.link("displayed altitude",
                   [=](double value){altitudeGauge->setValue(value*3.28084);});

    auto airspeedGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    airspeedGauge->setValueRange(0, 400);
    airspeedGauge->setAngleRange(-135, 90);
    airspeedGauge->setNumMajorTicks(9);
    airspeedGauge->addRangeBand(QColor("darkgreen"), 70, 180);
    airspeedGauge->addRangeBand(QColor("yellow"), 180, 310);
    airspeedGauge->addRangeBand(QColor("darkred"), 310, 350);
    airspeedGauge->setValueLabelPos(152, 220);
    airspeedGauge->setTextColor(QColor("white"));
    airspeedGauge->addLabel("Airspeed (kt)", 152, 200);
    m_updater.link("airspeed",
                   [=](double value){airspeedGauge->setValue(value*1.9438);});


    auto climbRateGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    climbRateGauge->setValueRange(-4000, 4000);
    climbRateGauge->setAngleRange(-135, 135);
    climbRateGauge->setNumMajorTicks(9);
    climbRateGauge->setValueLabelPos(152, 220);
    climbRateGauge->setTextColor(QColor("white"));
    climbRateGauge->addLabel("Climb Rate (fpm)", 152, 200);
    m_updater.link("vertical speed",
                   [=](double value){climbRateGauge->setValue(value*60);});

    auto column1Layout = new QVBoxLayout;
    column1Layout->addWidget(chtGroupBox);
    column1Layout->addWidget(egtGroupBox);

    auto column2Layout = new QVBoxLayout;
    column2Layout->addWidget(mapGauge);
    column2Layout->addWidget(oilTempGauge);
    column2Layout->addWidget(fuelLevel1Gauge);

    auto column3Layout = new QVBoxLayout;
    column3Layout->addWidget(rpmGauge);
    column3Layout->addWidget(oilPressGauge);
    column3Layout->addWidget(fuelLevel2Gauge);

    auto column4Layout = new QVBoxLayout;
    column4Layout->addWidget(fuelPressGauge);
    column4Layout->addWidget(fuelFlowGauge);
    column4Layout->addWidget(lambdaGauge);

    auto column5Layout = new QVBoxLayout;
    column5Layout->addWidget(altitudeGauge);
    column5Layout->addWidget(airspeedGauge);
    column5Layout->addWidget(climbRateGauge);

    auto mainLayout = new QHBoxLayout;
    mainLayout->addItem(column1Layout);
    mainLayout->addItem(column2Layout);
    mainLayout->addItem(column3Layout);
    mainLayout->addItem(column4Layout);
    mainLayout->addItem(column5Layout);
    
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
