QT += core gui serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = telemetry
TEMPLATE = app

SOURCES += main.cpp MainWindow.cpp TelemetryStream.cpp
HEADERS += MainWindow.h TelemetryStream.h

exists(../qtserialport) {
    QTSERIALPORT_BUILD_ROOT = ../qtserialport
    include(../qtserialport/src/serialport/qt4support/serialport.prf)
}
