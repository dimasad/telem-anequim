#ifndef TELEMETRYSTREAM_H
#define TELEMETRYSTREAM_H

#include <QtSerialPort/QSerialPort>
#include <QList>
#include <QTime>
#include <QTextStream>


class TelemetryVariable {
public:
    TelemetryVariable() {}
    TelemetryVariable(const QString & label, const QString & units, 
		      double value) : 
	label(label), units(units), value(value) {}
    operator QString() const;
    
    QString label, units;
    double value;
};


typedef QList<TelemetryVariable> TelemetryMessage;

    
class TelemetryStream : public QObject
{
    Q_OBJECT
    
public:
    TelemetryStream(const QString & portName, int message_body_size);
    
protected:
    QSerialPort port;
    int message_body_size, total_message_size;
    double parseDouble(int & cursor, unsigned len, const QByteArray & body);
    long parseHex(int & cursor, unsigned len, const QByteArray & body);
    virtual bool messageValid(quint8 checksum, const QByteArray & payload) = 0;
    virtual TelemetryMessage parseMessage(const QByteArray & body) = 0;

public slots:
    void triggerRead();

signals:
    void variableUpdated(const TelemetryVariable & var);
};


class EmsStream : public TelemetryStream
{
    Q_OBJECT
    
public:
    EmsStream(const QString & portName);
    bool parseGeneralPurpose(int & cursor, const QByteArray & body, 
                             TelemetryVariable & var);
    virtual bool messageValid(quint8 checksum, const QByteArray & payload);
    virtual TelemetryMessage parseMessage(const QByteArray & body);
};


class EfisStream : public TelemetryStream
{
    Q_OBJECT
    
public:
    EfisStream(const QString & portName);
    virtual bool messageValid(quint8 checksum, const QByteArray & payload);
    virtual TelemetryMessage parseMessage(const QByteArray & body);
};


class TelemetryDump : public QObject
{
    Q_OBJECT

public:
    TelemetryDump();

protected:
    QTextStream stream;
    
public slots:
    void printVariable(const TelemetryVariable & var);
};


#endif // TELEMETRYSTREAM_H
