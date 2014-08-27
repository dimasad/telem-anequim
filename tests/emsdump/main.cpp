#include "EmsDump.hpp"

#include <QtCore>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>


int main(int argc, char *argv[])
{

    QCoreApplication coreApplication(argc, argv);
    int argumentCount = QCoreApplication::arguments().size();
    QStringList argumentList = QCoreApplication::arguments();
    
    QTextStream standardOutput(stdout);
    if (argumentCount == 1 || argumentCount > 2) {
	QString msg("Usage: %1 <serialportname>");
	
        standardOutput << msg.arg(argumentList.first()) << endl;
        return 1;
    }
    
    

    return coreApplication.exec();
}
