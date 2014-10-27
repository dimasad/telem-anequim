QT += core gui widgets serialport svg

TARGET = telemetry
TEMPLATE = app

SOURCES += main.cpp MainWindow.cpp TelemetryStream.cpp Gauge.cpp
HEADERS += MainWindow.hpp TelemetryStream.hpp Gauge.hpp

RESOURCES += telemetryresources.qrc
