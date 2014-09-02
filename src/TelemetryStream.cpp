#include "TelemetryStream.hpp"

#include <cmath>
#include <QDebug>


#define EMS_MESSAGE_BODY_SIZE 119
#define EFIS_MESSAGE_BODY_SIZE 51
#define MESSAGE_FOOTER_SIZE 2


TelemetryVariable::operator QString() const
{
    return QString("%1 = %2 %3").arg(label).arg(value).arg(units);
}


TelemetryStream::TelemetryStream(const QString & portName,
                                 int message_body_size) :
    port(portName), message_body_size(message_body_size)
{
    total_message_size = message_body_size + MESSAGE_FOOTER_SIZE;
    
    connect(&port, SIGNAL(readyRead()), this, SLOT(triggerRead()));
    port.open(QIODevice::ReadOnly);
    port.setBaudRate(QSerialPort::Baud115200);
    port.setReadBufferSize(total_message_size);
}


void
TelemetryStream::triggerRead()
{
    //Check if the message end marker (CR + LF) has been reached
    if (!port.canReadLine())
	return;
    
    //Get the message body
    QByteArray line = port.readLine();
    if (line.endsWith("\r\n"))
        line.chop(2);
    if (line.size() < message_body_size)
	return;
    else if (line.size() > message_body_size)
	line = line.right(message_body_size);
    
    //Extract the checksum
    quint8 checksum = line.right(2).toInt(NULL, 16);
    line.chop(2);
    
    //Check the message
    for (int i = 0; i < line.size(); i++)
	checksum += line[i];
    if (checksum != 0)
	return;
    
    TelemetryMessage msg = parseMessage(line);
    for (TelemetryMessage::iterator i = msg.begin(); i != msg.end(); i++)
	emit variableUpdated(*i);
}


double
TelemetryStream::parseDouble(int & cursor, unsigned len,
                             const QByteArray & body)
{

    bool ok;
    double value = body.mid(cursor, len).toDouble(&ok);
    if (!ok)
	value = NAN;

    cursor += len;
    return value;
}


long
TelemetryStream::parseHex(int & cursor, unsigned len, const QByteArray & body)
{

    long value = body.mid(cursor, len).toInt(NULL, 16);
    cursor += len;
    return value;
}


EmsStream::EmsStream(const QString & portName) :
    TelemetryStream(portName, EMS_MESSAGE_BODY_SIZE)
{
}


TelemetryMessage
EmsStream::parseMessage(const QByteArray & body)
{
    TelemetryMessage msg;
    int cursor = 0;

    QString fahrenheit = QString::fromUtf8("\u00B0F");
    
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
            "oil temperature", fahrenheit, parseDouble(cursor, 3, body)
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

    // Skip general purpose variables
    cursor += 3 * 8;

    msg.append(
        TelemetryVariable(
            "general purpose thermocouple", fahrenheit,
	    parseDouble(cursor, 4, body)
        )
    );
    msg.append(
        TelemetryVariable("egt1", fahrenheit, parseDouble(cursor, 4, body))
    );
    msg.append(
        TelemetryVariable("egt2", fahrenheit, parseDouble(cursor, 4, body))
    );
    msg.append(
        TelemetryVariable("egt3", fahrenheit, parseDouble(cursor, 4, body))
    );
    msg.append(
        TelemetryVariable("egt4", fahrenheit, parseDouble(cursor, 4, body))
    );
    msg.append(
        TelemetryVariable("egt5", fahrenheit, parseDouble(cursor, 4, body))
    );
    msg.append(
        TelemetryVariable("egt6", fahrenheit, parseDouble(cursor, 4, body))
    );
    msg.append(
        TelemetryVariable("cht1", fahrenheit, parseDouble(cursor, 3, body))
    );
    msg.append(
        TelemetryVariable("cht2", fahrenheit, parseDouble(cursor, 3, body))
    );
    msg.append(
        TelemetryVariable("cht3", fahrenheit, parseDouble(cursor, 3, body))
    );
    msg.append(
        TelemetryVariable("cht4", fahrenheit, parseDouble(cursor, 3, body))
    );
    msg.append(
        TelemetryVariable("cht5", fahrenheit, parseDouble(cursor, 3, body))
    );
    msg.append(
        TelemetryVariable("cht6", fahrenheit, parseDouble(cursor, 3, body))
    );
    msg.append(
        TelemetryVariable("contact 1", "", parseDouble(cursor, 1, body))
    );
    msg.append(
        TelemetryVariable("contact 2", "", parseDouble(cursor, 1, body))
    );
    
    return msg;
}


EfisStream::EfisStream(const QString & portName) :
    TelemetryStream(portName, EFIS_MESSAGE_BODY_SIZE)
{
}


TelemetryMessage
EfisStream::parseMessage(const QByteArray & body)
{
    TelemetryMessage msg;
    int cursor = 0;

    QString degrees = QString::fromUtf8("\u00B0");
    QString degrees_per_second = QString::fromUtf8("\u00B0/s");
    
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
        TelemetryVariable("pitch", degrees, parseDouble(cursor, 4, body) / 10)
    );
    msg.append(
        TelemetryVariable("roll", degrees, parseDouble(cursor, 5, body) / 10)
    );
    msg.append(
        TelemetryVariable("yaw", degrees, parseDouble(cursor, 3, body))
    );
    msg.append(
        TelemetryVariable("airspeed", "m/s", parseDouble(cursor, 4, body) / 10)
    );

    double altitude_alternating = parseDouble(cursor, 4, body);
    double vsi_alternating = parseDouble(cursor, 4, body) / 10;

    msg.append(
        TelemetryVariable(
            "lateral acceleration", "g", parseDouble(cursor, 3, body) / 100
        )
    );
    msg.append(
        TelemetryVariable(
            "vertical acceleration", "g", parseDouble(cursor, 3, body) / 10
        )
    );
    msg.append(
        TelemetryVariable(
            "angle of attack", "% of stall", parseDouble(cursor, 2, body)
        )
    );

    int status_bitmask = parseHex(cursor, 6, body);
    if (status_bitmask & 1) {
        msg.append(
            TelemetryVariable("pressure altitude", "m", altitude_alternating)
        );
        msg.append(
            TelemetryVariable("turn rate", degrees_per_second, vsi_alternating)
        );
    } else {
        msg.append(
            TelemetryVariable("displayed altitude", "m", altitude_alternating)
        );
        msg.append(
            TelemetryVariable("vertical speed", "ft/s", vsi_alternating)
        );
    }

    
    return msg;
}


TelemetryDump::TelemetryDump() :
    stream(stdout)
{
}


void
TelemetryDump::printVariable(const TelemetryVariable & var)
{
    stream << (QString) var << endl;
}
