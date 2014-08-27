#ifndef TELEMETRYSTREAM_H
#define TELEMETRYSTREAM_H

#include <QtSerialPort/QSerialPort>
#include <QList>
#include <QTime>
#include <QTextStream>


class TelemetryVariable {
public:
    TelemetryVariable(const QString & label, const QString & units, 
		      double value) : 
	label(label), units(units), value(value)
    {}
    operator QString() const;
    
    QString label, units;
    double value;
};


typedef QList<TelemetryVariable> TelemetryMessage;

    
class EmsStream : public QObject
{
    Q_OBJECT
    
public:
    EmsStream(const QString & portName);
    
private:
    QSerialPort port;
    
    TelemetryMessage parseMessage(const QByteArray & body);
    double parseDouble(int & cursor, unsigned len, const QByteArray & body);

public slots:
    void triggerRead();

signals:
    void variableUpdated(const TelemetryVariable & var);
};


class TelemetryDump : public QObject
{
    Q_OBJECT

public:
    TelemetryDump();

private:
    QTextStream stream;
    
public slots:
    void printVariable(const TelemetryVariable & var);
};


#endif // TELEMETRYSTREAM_H
