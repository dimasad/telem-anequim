#include "MainWindow.hpp"

#include <QtGui>
#include <QAction>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSerialPortInfo>
#include <QStatusBar>
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
    m_logFolder = m_storedSettings.value("log_folder").toString();
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
Settings::setLogFolder(const QString &newLogFolder)
{
    m_logFolder = newLogFolder;
    m_storedSettings.setValue("log_folder", newLogFolder);
    
    emit logFolderChanged(newLogFolder);
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
    
    m_efisPortComboBox = new QComboBox;
    m_emsPortComboBox = new QComboBox;
    m_efisPortComboBox->addItems(portNames);
    m_emsPortComboBox->addItems(portNames);
    setCurrentPort(settings->emsPort(), m_emsPortComboBox);
    setCurrentPort(settings->efisPort(), m_efisPortComboBox);

    m_logFolderButton = new QPushButton(settings->logFolder());
    connect(m_logFolderButton, SIGNAL(clicked()), 
            this, SLOT(chooseLogFolder()));
    
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                          | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    auto layout = new QFormLayout;
    layout->addRow("EFIS port:", m_efisPortComboBox);
    layout->addRow("EMS port:", m_emsPortComboBox);
    layout->addRow("Log folder:", m_logFolderButton);
    layout->addRow(buttonBox);
    setLayout(layout);
    
    connect(this, SIGNAL(accepted()), this, SLOT(saveSettings()));
}


void
SettingsDialog::chooseLogFolder()
{
    QFileDialog chooser(this);
    chooser.setFileMode(QFileDialog::Directory);
    if (chooser.exec())
        m_logFolderButton->setText(chooser.selectedFiles().first());
}


void
SettingsDialog::saveSettings()
{
    m_settings->setEfisPort(m_efisPortComboBox->currentText());
    m_settings->setEmsPort(m_emsPortComboBox->currentText());
    m_settings->setLogFolder(m_logFolderButton->text());
    m_settings->sync();
}


void
SettingsDialog::setCurrentPort(const QString &portName, QComboBox *comboBox)
{
    int index = comboBox->findText(portName);
    if (index == -1) {
        comboBox->insertItem(0, portName);
        index = 0;
    }
    comboBox->setCurrentIndex(index);
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

    m_efisStatusLabel = new QLabel("EFIS offline.");
    m_emsStatusLabel = new QLabel("EMS offline.");
    statusBar()->addWidget(m_efisStatusLabel);
    statusBar()->addWidget(m_emsStatusLabel);

    m_efisStatusTimer = new QTimer(this);
    m_efisStatusTimer->setInterval(1000);
    m_efisStatusTimer->setSingleShot(true);
    connect(m_efisStream, SIGNAL(messageReceived(const TelemetryMessage &)),
            m_efisStatusTimer, SLOT(start()));
    connect(m_efisStream, SIGNAL(messageReceived(const TelemetryMessage &)),
            this, SLOT(efisOnline()));
    connect(m_efisStatusTimer, SIGNAL(timeout()),
            this, SLOT(efisOffline()));

    m_emsStatusTimer = new QTimer(this);
    m_emsStatusTimer->setInterval(1000);
    m_emsStatusTimer->setSingleShot(true);
    connect(m_emsStream, SIGNAL(messageReceived(const TelemetryMessage &)),
            m_emsStatusTimer, SLOT(start()));
    connect(m_emsStream, SIGNAL(messageReceived(const TelemetryMessage &)),
            this, SLOT(emsOnline()));
    connect(m_emsStatusTimer, SIGNAL(timeout()),
            this, SLOT(emsOffline()));
    
    auto rpmGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    rpmGauge->setValueRange(0, 3500);
    rpmGauge->setAngleRange(-135, 90);
    rpmGauge->setNumMajorTicks(8);
    rpmGauge->setTextColor(QColor("white"));
    rpmGauge->addRangeBand(QColor("darkgreen"), 600, 2700);
    rpmGauge->addRangeBand(QColor("goldenrod"), 2700, 3200);
    rpmGauge->addRangeBand(QColor("darkred"), 3200, 3500);
    rpmGauge->setBottomLabel("RPM");
    m_updater.link("RPM", [=](double value){rpmGauge->setValue(value);});
    
    auto cht1Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht1Gauge->setValueRange(150, 500);
    cht1Gauge->setNumMajorTicks(8);
    cht1Gauge->setTextColor(QColor("white"));
    cht1Gauge->addRangeBand(QColor("darkred"), 0, 150);
    cht1Gauge->addRangeBand(QColor("goldenrod"), 150, 200);
    cht1Gauge->addRangeBand(QColor("darkgreen"), 200, 435);
    cht1Gauge->addRangeBand(QColor("goldenrod"), 435, 450);
    cht1Gauge->addRangeBand(QColor("darkred"), 450, 500);
    m_updater.link("cht1", [=](double value){cht1Gauge->setValue(value);});

    auto cht2Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht2Gauge->setValueRange(150, 500);
    cht2Gauge->setNumMajorTicks(8);
    cht2Gauge->setTextColor(QColor("white"));
    cht2Gauge->addRangeBand(QColor("darkred"), 0, 150);
    cht2Gauge->addRangeBand(QColor("goldenrod"), 150, 200);
    cht2Gauge->addRangeBand(QColor("darkgreen"), 200, 435);
    cht2Gauge->addRangeBand(QColor("goldenrod"), 435, 450);
    cht2Gauge->addRangeBand(QColor("darkred"), 450, 500);
    m_updater.link("cht2", [=](double value){cht2Gauge->setValue(value);});

    auto cht3Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht3Gauge->setValueRange(150, 500);
    cht3Gauge->setNumMajorTicks(8);
    cht3Gauge->setTextColor(QColor("white"));
    cht3Gauge->addRangeBand(QColor("darkred"), 0, 150);
    cht3Gauge->addRangeBand(QColor("goldenrod"), 150, 200);
    cht3Gauge->addRangeBand(QColor("darkgreen"), 200, 435);
    cht3Gauge->addRangeBand(QColor("goldenrod"), 435, 450);
    cht3Gauge->addRangeBand(QColor("darkred"), 450, 500);
    m_updater.link("cht3", [=](double value){cht3Gauge->setValue(value);});

    auto cht4Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    cht4Gauge->setValueRange(150, 500);
    cht4Gauge->setNumMajorTicks(8);
    cht4Gauge->setTextColor(QColor("white"));
    cht4Gauge->addRangeBand(QColor("darkred"), 0, 150);
    cht4Gauge->addRangeBand(QColor("goldenrod"), 150, 200);
    cht4Gauge->addRangeBand(QColor("darkgreen"), 200, 435);
    cht4Gauge->addRangeBand(QColor("goldenrod"), 435, 450);
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
    oilPressGauge->addRangeBand(QColor("goldenrod"), 15, 20);
    oilPressGauge->addRangeBand(QColor("darkgreen"), 20, 90);
    oilPressGauge->addRangeBand(QColor("goldenrod"), 90, 95);
    oilPressGauge->addRangeBand(QColor("darkred"), 95, 100);
    oilPressGauge->setTextColor(QColor("white"));
    oilPressGauge->setBottomLabel("Oil press.");
    oilPressGauge->setTopLabel("psi");
    m_updater.link("oil pressure",
                   [=](double value){oilPressGauge->setValue(value);});
 
    auto mapGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    mapGauge->setValueRange(0, 40);
    mapGauge->setAngleRange(-135, 90);
    mapGauge->setNumMajorTicks(6);
    mapGauge->setNumMinorTicks(3);
    mapGauge->addRangeBand(QColor("darkgreen"), 0, 36);
    mapGauge->addRangeBand(QColor("goldenrod"), 36, 38);
    mapGauge->addRangeBand(QColor("darkred"), 38, 40);
    mapGauge->setTextColor(QColor("white"));
    mapGauge->setBottomLabel("MAP");
    mapGauge->setTopLabel("inHg");
    m_updater.link("manifold pressure",
                   [=](double value){mapGauge->setValue(value);});
    
    auto egt1Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt1Gauge->setValueRange(800, 1600);
    egt1Gauge->setNumMajorTicks(5);
    egt1Gauge->setNumMinorTicks(3);
    egt1Gauge->setTextColor(QColor("white"));
    egt1Gauge->addRangeBand(QColor("darkgreen"), 400, 1500);
    egt1Gauge->addRangeBand(QColor("goldenrod"), 1500, 1600);
    egt1Gauge->setValue(1600);
    m_updater.link("egt1", [=](double value){egt1Gauge->setValue(value);});

    auto egt2Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt2Gauge->setValueRange(800, 1600);
    egt2Gauge->setNumMajorTicks(5);
    egt2Gauge->setNumMinorTicks(3);
    egt2Gauge->setTextColor(QColor("white"));
    egt2Gauge->addRangeBand(QColor("darkgreen"), 400, 1500);
    egt2Gauge->addRangeBand(QColor("goldenrod"), 1500, 1600);
    m_updater.link("egt2", [=](double value){egt2Gauge->setValue(value);});

    auto egt3Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt3Gauge->setValueRange(800, 1600);
    egt3Gauge->setNumMajorTicks(5);
    egt3Gauge->setNumMinorTicks(3);
    egt3Gauge->setTextColor(QColor("white"));
    egt3Gauge->addRangeBand(QColor("darkgreen"), 400, 1500);
    egt3Gauge->addRangeBand(QColor("goldenrod"), 1500, 1600);
    m_updater.link("egt3", [=](double value){egt3Gauge->setValue(value);});

    auto egt4Gauge = new LinearSvgGauge(":/images/horizontal-gauge.svg");
    egt4Gauge->setValueRange(800, 1600);
    egt4Gauge->setNumMajorTicks(5);
    egt4Gauge->setNumMinorTicks(3);
    egt4Gauge->setTextColor(QColor("white"));
    egt4Gauge->addRangeBand(QColor("darkgreen"), 400, 1500);
    egt4Gauge->addRangeBand(QColor("goldenrod"), 1500, 1600);
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
    oilTempGauge->addRangeBand(QColor("goldenrod"), 60, 100);
    oilTempGauge->addRangeBand(QColor("darkgreen"), 100, 220);
    oilTempGauge->addRangeBand(QColor("goldenrod"), 220, 240);
    oilTempGauge->addRangeBand(QColor("darkred"), 240, 260);
    oilTempGauge->setTextColor(QColor("white"));
    oilTempGauge->setBottomLabel("Oil temp.");
    oilTempGauge->setTopLabel("\u00B0F");
    m_updater.link("oil temperature",
                   [=](double value){oilTempGauge->setValue(value);});
    
    auto fuelPressGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelPressGauge->setValueRange(0, 30);
    fuelPressGauge->setAngleRange(-135, 90);
    fuelPressGauge->setNumMajorTicks(7);
    fuelPressGauge->addRangeBand(QColor("darkred"), 0, 10);
    fuelPressGauge->addRangeBand(QColor("goldenrod"), 10, 15);
    fuelPressGauge->addRangeBand(QColor("darkgreen"), 15, 27);
    fuelPressGauge->addRangeBand(QColor("goldenrod"), 27, 28);
    fuelPressGauge->addRangeBand(QColor("darkred"), 28, 30);
    fuelPressGauge->setTextColor(QColor("white"));
    fuelPressGauge->setBottomLabel("Fuel pressure");
    fuelPressGauge->setTopLabel("psi");
    m_updater.link("fuel pressure",
                   [=](double value){fuelPressGauge->setValue(value);});

    auto fuelLevel1Gauge = new AngularSvgGauge(":/images/top-circle-gauge.svg");
    fuelLevel1Gauge->setValueRange(0, 24);
    fuelLevel1Gauge->setAngleRange(-80, 80);
    fuelLevel1Gauge->setNumMajorTicks(7);
    fuelLevel1Gauge->addRangeBand(QColor("darkred"), 0, 1.3);
    fuelLevel1Gauge->addRangeBand(QColor("goldenrod"), 1.3, 1.5);
    fuelLevel1Gauge->addRangeBand(QColor("darkgreen"), 1.5, 24);
    fuelLevel1Gauge->setTextColor(QColor("white"));
    fuelLevel1Gauge->setTopLabel("Fuel 1 (gal)");
    //fuelLevel1Gauge->setTopLabel("gal");
    m_updater.link("fuel level 1",
                   [=](double value){fuelLevel1Gauge->setValue(value);});


    auto fuelLevel2Gauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelLevel2Gauge->setValueRange(0, 4);
    fuelLevel2Gauge->setAngleRange(-135, 90);
    fuelLevel2Gauge->setNumMajorTicks(6);
    fuelLevel2Gauge->addRangeBand(QColor("darkred"), 0, 1.3);
    fuelLevel2Gauge->addRangeBand(QColor("goldenrod"), 1.3, 1.5);
    fuelLevel2Gauge->addRangeBand(QColor("darkgreen"), 1.5, 4);
    fuelLevel2Gauge->setTextColor(QColor("white"));
    fuelLevel2Gauge->setBottomLabel("Fuel level 2");
    fuelLevel2Gauge->setTopLabel("gal");
    m_updater.link("fuel level 2",
                   [=](double value){fuelLevel2Gauge->setValue(value);});

    auto fuelFlowGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    fuelFlowGauge->setValueRange(0, 25);
    fuelFlowGauge->setAngleRange(-135, 90);
    fuelFlowGauge->setNumMajorTicks(6);
    fuelFlowGauge->addRangeBand(QColor("darkgreen"), 0, 22);
    fuelFlowGauge->addRangeBand(QColor("goldenrod"), 22, 25);
    fuelFlowGauge->setTextColor(QColor("white"));
    fuelFlowGauge->setBottomLabel("Fuel flow");
    fuelFlowGauge->setTopLabel("gal/h");
    m_updater.link("fuel flow",
                   [=](double value){fuelFlowGauge->setValue(value);});

    auto lambdaGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    lambdaGauge->setValueRange(0, 100);
    lambdaGauge->setAngleRange(-135, 90);
    lambdaGauge->setNumMajorTicks(11);
    lambdaGauge->addRangeBand(QColor("darkgreen"), 30, 35);
    lambdaGauge->setTextColor(QColor("white"));
    lambdaGauge->setBottomLabel("Lambda");
    lambdaGauge->setTopLabel("%");
    m_updater.link("aileron trim",
                   [=](double value){lambdaGauge->setValue(value);});
    
    auto altitudeGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    altitudeGauge->setValueRange(0, 10000);
    altitudeGauge->setAngleRange(-135, 90);
    altitudeGauge->setNumMajorTicks(11);
    altitudeGauge->setTextColor(QColor("white"));
    altitudeGauge->setBottomLabel("Altitude");
    altitudeGauge->setTopLabel("ft");
    m_updater.link("displayed altitude",
                   [=](double value){altitudeGauge->setValue(value*3.28084);});

    auto airspeedGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    airspeedGauge->setValueRange(0, 400);
    airspeedGauge->setAngleRange(-135, 90);
    airspeedGauge->setNumMajorTicks(9);
    airspeedGauge->addRangeBand(QColor("darkgreen"), 70, 180);
    airspeedGauge->addRangeBand(QColor("goldenrod"), 180, 310);
    airspeedGauge->addRangeBand(QColor("darkred"), 310, 350);
    airspeedGauge->setTextColor(QColor("white"));
    airspeedGauge->setBottomLabel("Airspeed");
    airspeedGauge->setTopLabel("kt");
    m_updater.link("airspeed",
                   [=](double value){airspeedGauge->setValue(value*1.9438);});


    auto climbRateGauge = new AngularSvgGauge(":/images/angular-gauge.svg");
    climbRateGauge->setValueRange(-4000, 4000);
    climbRateGauge->setAngleRange(-135, 135);
    climbRateGauge->setNumMajorTicks(9);
    climbRateGauge->setTextColor(QColor("white"));
    climbRateGauge->setBottomLabel("Climb Rate");
    climbRateGauge->setTopLabel("fpm");
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
MainWindow::efisOnline()
{
    m_efisStatusLabel->setText("EFIS online.");
}


void
MainWindow::efisOffline()
{
    m_efisStatusLabel->setText("EFIS offline.");
}


void
MainWindow::emsOnline()
{
    m_emsStatusLabel->setText("EMS online.");
}


void
MainWindow::emsOffline()
{
    m_emsStatusLabel->setText("EMS offline.");
}


void
MainWindow::showSettingsDialog()
{
    SettingsDialog settingsDialog(&m_settings);
    settingsDialog.exec();
}
