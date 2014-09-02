QT += core
greaterThan(QT_MAJOR_VERSION, 4): QT += serialport

INCLUDEPATH += ../../src

TARGET = telemetrydump
TEMPLATE = app

SOURCES += main.cpp ../../src/TelemetryStream.cpp
HEADERS += ../../src/TelemetryStream.hpp

exists(../../qtserialport) {
    QTSERIALPORT_BUILD_ROOT = ../../qtserialport
    include(../../qtserialport/src/serialport/qt4support/serialport.prf)
    QMAKE_LFLAGS += -Wl,-rpath=$$OUT_PWD/../../qtserialport/src/serialport
}
