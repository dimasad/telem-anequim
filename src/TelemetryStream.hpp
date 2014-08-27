#ifndef TELEMETRYSTREAM_H
#define TELEMETRYSTREAM_H


#include <QList>
#include <QTime>
#include <QtSerialPort/QSerialPort>


class TelemetryVariable {
public:
    TelemetryVariable(const QString & label, const QString & units, 
		      double value) : 
	label(label), units(units), value(value)
    {}
    
    QString label, units;
    double value;
};


typedef QList<TelemetryVariable> TelemetryMessage;

    
class EmsStream : public QObject
{
    Q_OBJECT
    
public:
    EmsStream(const QString & portName);
    
    QSerialPort port;

private:
    TelemetryMessage parseMessage(const QByteArray & body);
    double parseDouble(int & cursor, unsigned len, const QByteArray & body);

public slots:
    void triggerRead();

signals:
    void variableUpdated(const TelemetryVariable & var);
};


#endif // TELEMETRYSTREAM_H
