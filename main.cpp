#include "mainwindow.h"

#include <QApplication>

#include <qserialport.h>

#include <QSerialPortInfo>

#include <QFile>

#include <QDebug>

#include "DarkeumStyle.h"

int main(int argc, char *argv[])
{





    QApplication a(argc, argv);

    //QFile file("/Users/james/Documents/GitHub/Grid-Simulator-Control-Interface/Darkeum.qss");


    MainWindow w;
    QSerialPort serial;

    //serial.setPortName("COM19");

    a.setStyleSheet(StyleSheet);
    /*
    if (file.open(QFile::ReadOnly)) {

        QString styleSheet = QLatin1String(file.readAll());


        a.setStyleSheet(StyleSheet);
        file.close();
    }
    else{qDebug() << "error in files";}
*/
    w.show();
    return a.exec();
}
