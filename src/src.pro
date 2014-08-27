QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport

TARGET = telemetry
TEMPLATE = app

SOURCES += main.cpp MainWindow.cpp TelemetryStream.cpp
HEADERS += MainWindow.hpp TelemetryStream.hpp

exists(../qtserialport) {
    QTSERIALPORT_BUILD_ROOT = ../qtserialport
    include(../qtserialport/src/serialport/qt4support/serialport.prf)
    QMAKE_LFLAGS += -Wl,-rpath=$$OUT_PWD/../qtserialport/src/serialport
}
