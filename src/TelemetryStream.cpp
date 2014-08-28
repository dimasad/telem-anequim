#include "TelemetryStream.hpp"

#include <cmath>
#include <QDebug>

#define MSG_BODY_SIZE 119
#define MSG_FOOTER_SIZE 2
#define MSG_TOTAL_SIZE (MSG_BODY_SIZE + MSG_FOOTER_SIZE)


TelemetryVariable::operator QString() const
{
    return QString("%1 = %2 %3").arg(label).arg(value).arg(units);
}


EmsStream::EmsStream(const QString & portName) : 
    port(portName)
{
    port.setBaudRate(QSerialPort::Baud115200);
    port.setReadBufferSize(MSG_TOTAL_SIZE);
    connect(&port, SIGNAL(readyRead()), this, SLOT(triggerRead()));
    port.open(QIODevice::ReadOnly);
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


TelemetryDump::TelemetryDump() :
    stream(stdout)
{
}


void
TelemetryDump::printVariable(const TelemetryVariable & var)
{
    stream << (QString) var << endl;
}
