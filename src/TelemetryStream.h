#ifndef TELEMETRYSTREAM_H
#define TELEMETRYSTREAM_H

#include <QtSerialPort/QSerialPort>
#include <QTime>

/// General purpose field of the EMS message
class EmsGP {
public:
    QString label;
    double value;
};


class EmsMessage {
public:
    EmsMessage(const QByteArray & body);
    
    QTime time;
    double manifoldPressure, oilTemperature, oilPressure, fuelPressure,
	volts, amps, rpm, fuelFlow, gallonsRemaining, fuelLevel1, fuelLevel2,
	gpThermocouple, egt[6], cht[6];
    EmsGP gp[3];
    bool contact[2];
    char productId[2];
};

    
class EmsStream : public QObject
{
    Q_OBJECT
    
public:
    EmsStream(const QString & name);
    
    QSerialPort port;

public slots:
    void triggerRead();

signals:
    void messageReceived(EmsMessage & msg);
};


#endif // TELEMETRYSTREAM_H
