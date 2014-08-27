#include "TelemetryStream.hpp"
#include <cmath>
#include <QLocale>

#define MSG_BODY_SIZE 119
#define MSG_FOOTER_SIZE 2
#define MSG_TOTAL_SIZE (MSG_BODY_SIZE + MSG_FOOTER_SIZE)


EmsStream::EmsStream(const QString & portName) : 
    port(portName)
{
    port.setReadBufferSize(MSG_TOTAL_SIZE);
    connect(&port, SIGNAL(readyRead()), this, SLOT(triggerRead()));
}


void
EmsStream::triggerRead()
{
    //Check if the message end marker (CR + LF) has been reached
    if (!port.canReadLine())
	return;
    
    //Get the message body
    QByteArray line = port.readLine();
    if (line.size() < MSG_BODY_SIZE)
	return;
    else if (line.size() > MSG_BODY_SIZE)
	line = line.left(MSG_BODY_SIZE);

    //Evaluate the checksum
    quint16 checksum = 0;
    for (int i = 0; i < MSG_BODY_SIZE; i++)
	checksum += line[i];
    if (checksum != 0)
	return;

    TelemetryMessage msg = parseMessage(line);
    //emit messageReceived(msg);
}


TelemetryMessage
EmsStream::parseMessage(const QByteArray & body)
{
    TelemetryMessage msg;
    int cursor = 0;
    
    msg.append(
        TelemetryVariable("hour", "h", parseDouble(cursor, 2, body))
    );
    msg.append(
        TelemetryVariable("minute", "min", parseDouble(cursor, 2, body))
    );
    msg.append(
        TelemetryVariable("second", "s", parseDouble(cursor, 2, body))
    );
    msg.append(
        TelemetryVariable(
            "millisecond", "ms", parseDouble(cursor, 2, body) / 64
        )
    );
    msg.append(
        TelemetryVariable(
            "manifold pressure", "inHg", parseDouble(cursor, 4, body) / 100
        )
    );
    msg.append(
        TelemetryVariable(
            "oil temperature", "\uC2B0F", parseDouble(cursor, 3, body)
        )
    );
    msg.append(
        TelemetryVariable("oil pressure", "PSI", parseDouble(cursor, 3, body))
    );
    msg.append(
        TelemetryVariable(
            "fuel pressure", "PSI", parseDouble(cursor, 3, body) / 10
        )
    );
    msg.append(
        TelemetryVariable("voltage", "V", parseDouble(cursor, 3, body) / 10)
    );
    msg.append(
        TelemetryVariable("current", "A", parseDouble(cursor, 3, body))
    );
    msg.append(
        TelemetryVariable("RPM", "RPM", parseDouble(cursor, 3, body) * 10)
    );
    msg.append(
        TelemetryVariable("fuel flow", "GPH", parseDouble(cursor, 3, body) / 10)
    );
    msg.append(
        TelemetryVariable(
            "remaining fuel", "gal", parseDouble(cursor, 4, body) / 10
        )
    );
    msg.append(
        TelemetryVariable(
            "fuel level 1", "gal", parseDouble(cursor, 3, body) / 10
        )
    );
    msg.append(
        TelemetryVariable(
            "fuel level 2", "gal", parseDouble(cursor, 3, body) / 10
        )
    );
    
    return msg;
}


double 
EmsStream::parseDouble(int & cursor, unsigned len, const QByteArray & body)
{
    
    bool ok;
    double value = body.mid(cursor, len).toDouble(&ok);
    if (!ok)
	value = NAN;

    cursor += len;
    return value;
}
