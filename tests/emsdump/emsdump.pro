QT += core
greaterThan(QT_MAJOR_VERSION, 4): QT += serialport

TARGET = emsdump
TEMPLATE = app

SOURCES += main.cpp EmsDump.cpp ../../src/TelemetryStream.cpp
HEADERS += EmsDump.hpp ../../src/TelemetryStream.hpp

exists(../../qtserialport) {
    QTSERIALPORT_BUILD_ROOT = ../../qtserialport
    include(../../qtserialport/src/serialport/qt4support/serialport.prf)
    QMAKE_LFLAGS += -Wl,-rpath=$$OUT_PWD/../../qtserialport/src/serialport
}
