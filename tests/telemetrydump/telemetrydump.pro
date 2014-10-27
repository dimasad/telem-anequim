QT += core serialport

INCLUDEPATH += ../../src

TARGET = telemetrydump
TEMPLATE = app

SOURCES += main.cpp ../../src/TelemetryStream.cpp
HEADERS += ../../src/TelemetryStream.hpp
