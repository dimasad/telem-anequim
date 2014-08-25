#include "TelemetryStream.h"

#define MSG_BODY_SIZE 119
#define MSG_FOOTER_SIZE 2
#define MSG_TOTAL_SIZE (MSG_BODY_SIZE + MSG_FOOTER_SIZE)


EmsMessage::EmsMessage(const QByteArray & body)
{
}


EmsStream::EmsStream(const QString & name) : 
    port(name)
{
    port.setReadBufferSize(MSG_TOTAL_SIZE);
    connect(&port, SIGNAL(readyRead()), this, SLOT(triggerRead()));
}


void EmsStream::triggerRead()
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

    EmsMessage msg(line);
    emit messageReceived(msg);
}
