#ifndef TELEMETRYSTREAM_H
#define TELEMETRYSTREAM_H

#include <QtSerialPort/QSerialPort>

class EmsStream : public QObject
{
    Q_OBJECT
    
 public:
    EmsStream(const QString & name);

    QSerialPort port;
    
 private:
};


#endif // TELEMETRYSTREAM_H
