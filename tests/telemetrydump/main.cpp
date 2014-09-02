#include "TelemetryStream.hpp"

#include <QtCore>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>


int main(int argc, char *argv[])
{

    QCoreApplication coreApplication(argc, argv);
    QTextStream standardOutput(stdout);

    // Get and check command-line arguments
    QStringList arguments = QCoreApplication::arguments();    
    if (arguments.size() != 3) {
	QString msg("Usage: %1 <serialportname> <ems|efis>");
	
        standardOutput << msg.arg(arguments.first()) << endl;
        return 1;
    }
    QString portName = arguments.at(1);
    QString streamType = arguments.at(2);
    if (streamType != "ems" && streamType != "efis") {
        QString msg("Error: unknown stream type '%1'");
        standardOutput << msg.arg(streamType)  << endl;
        return 1;
    }
    
    // Create the objects
    TelemetryStream *stream;
    if (arguments.at(2) == "ems") {
        stream = new EmsStream(portName);
    } else {
        stream = new EfisStream(portName);
    }
    TelemetryDump dump;
    QObject::connect(stream, 
		     SIGNAL(variableUpdated(const TelemetryVariable &)), 
		     &dump, 
		     SLOT(printVariable(const TelemetryVariable &)));
    
    return coreApplication.exec();
}
