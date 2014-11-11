#include "TelemetryStream.hpp"

#include <QVector>
#include <QDebug>

#include <cmath>
#include <string>


#define EMS_MESSAGE_BODY_SIZE 119
#define EFIS_MESSAGE_BODY_SIZE 51
#define MESSAGE_FOOTER_SIZE 2


static const QString fahrenheit = QString::fromUtf8("\u00B0F");
static const QString degrees = QString::fromUtf8("\u00B0");
static const QString degrees_per_second = QString::fromUtf8("\u00B0/s");


TelemetryVariable::operator QString() const
{
    return QString("%1 = %2 %3").arg(label).arg(value).arg(units);
}


TelemetryStream::TelemetryStream(const QString &portName,
                                 int message_body_size, QObject *parent) :
    QObject(parent), port(portName), message_body_size(message_body_size)
{
    total_message_size = message_body_size + MESSAGE_FOOTER_SIZE;

    setPort(portName);
    connect(&port, SIGNAL(readyRead()), this, SLOT(triggerRead()));
}


void
TelemetryStream::startLogging(const QString &logFileName)
{
    m_logFile = new QFile(logFileName, this);
    m_logFile->open(QIODevice::ReadOnly | QIODevice::Text);
}


void
TelemetryStream::stopLogging()
{
    delete m_logFile;
}


bool
TelemetryStream::isLoggingOn()
{
    return m_logFile != 0;
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
    if (!messageValid(checksum, line))
	return;
    
    TelemetryMessage msg = parseMessage(line);
    for (TelemetryMessage::iterator i = msg.begin(); i != msg.end(); i++)
	emit variableUpdated(*i);

    if (isLoggingOn())
        logMessage(msg);
}


void
TelemetryStream::setPort(const QString &portName)
{
    if (portName.isEmpty())
        return;
    
    if (port.isOpen())
        port.close();
    
    port.setPortName(portName);
    port.open(QIODevice::ReadOnly);
    port.setBaudRate(QSerialPort::Baud115200);
    port.setReadBufferSize(total_message_size);
}


void
TelemetryStream::includeInLog(const QString &variableName)
{
    m_logVariables.insert(variableName, m_logVariables.size());
}


void
TelemetryStream::logMessage(const TelemetryMessage &message)
{
    QVector<double> data(m_logVariables.size(), NAN);
    
    for (const auto &variable: message) {
        auto indexIterator = m_logVariables.find(variable.label);
        if (indexIterator != m_logVariables.end())
            data[*indexIterator] = variable.value;
    }
    
    for (auto datum: data) {
        m_logFile->write(std::to_string(datum).c_str());
        m_logFile->write("\t");
    }
    m_logFile->write("\n");
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


EmsStream::EmsStream(const QString & portName, QObject *parent) :
    TelemetryStream(portName, EMS_MESSAGE_BODY_SIZE, parent)
{
}


bool
EmsStream::parseGeneralPurpose(int & cursor, const QByteArray & body, 
                               TelemetryVariable & var)
{
    QByteArray label = body.mid(cursor, 3);
    cursor += 3;
    
    double value = parseDouble(cursor, 5, body);
    if (label == "OAT") {
        var.label = "OAT";
        var.value = value;
        var.units = fahrenheit;
    } else if (label == "CRB") {
        var.label = "carburator temperature";
        var.value = value;
        var.units = fahrenheit;
    } else if (label == "CLT") {
        var.label = "coolant temperature";
        var.value = value;
        var.units = fahrenheit;
    } else if (label == "CLP") {
        var.label = "coolant pressure";
        var.value = value;
        var.units = "psi";
    } else if (label == "FL3") {
        var.label = "fuel level 3";
        var.value = value / 10;
        var.units = "gal";
    } else if (label == "FL4") {
        var.label = "fuel level 4";
        var.value = value / 10;
        var.units = "gal";
    } else if (label == "CHT") {
        var.label = "cylinder head temperature";
        var.value = value;
        var.units = fahrenheit;
    } else if (label == "TRA") {
        var.label = "aileron trim";
        var.value = value;
        var.units = "%";
    } else if (label == "TRE") {
        var.label = "elevator trim";
        var.value = value;
        var.units = "%";
    } else if (label == "TRR") {
        var.label = "rudder trim";
        var.value = value;
        var.units = "%";
    } else if (label == "FLP") {
        var.label = "flap position";
        var.value = value;
        var.units = degrees;
    } else {
        return false;
    }
    
    return true;
}


bool
EmsStream::messageValid(quint8 checksum, const QByteArray & payload)
{
    for (int i = 0; i < payload.size(); i++)
        checksum += payload[i];
    return checksum == 0;
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

    for (int i = 0; i < 3; i++) {
        TelemetryVariable gp;
        if (parseGeneralPurpose(cursor, body, gp))
            msg.append(gp);
    }
    
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


EfisStream::EfisStream(const QString & portName, QObject *parent) :
    TelemetryStream(portName, EFIS_MESSAGE_BODY_SIZE, parent)
{
}


bool
EfisStream::messageValid(quint8 checksum, const QByteArray & payload)
{
    quint8 sum = 0;
    for (int i = 0; i < payload.size(); i++)
        sum += payload[i];
    
    return checksum == sum;
}


TelemetryMessage
EfisStream::parseMessage(const QByteArray & body)
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

    double altitude_alternating = parseDouble(cursor, 5, body);
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

/***** EMS
"hour""minute""second""millisecond""manifold pressure""oil temperature""oil pressure""fuel pressure""voltage""current""RPM""fuel flow""remaining fuel""fuel level 1""fuel level 2""general purpose thermocouple""egt1""egt2""egt3""egt4""egt5""egt6""cht1""cht2""cht3""cht4""cht5""cht6""contact 1""contact 2"
****/


/****** EFIS
"hour""minute""second""millisecond""pitch""roll""yaw""airspeed""lateral acceleration""vertical acceleration""angle of attack""pressure altitude""turn rate""displayed altitude""vertical speed"
****/
