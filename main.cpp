#include "mainwindow.h"

#include <QApplication>

#include <qserialport.h>

#include <QSerialPortInfo>

#include <QFile>

int main(int argc, char *argv[])
{





    QApplication a(argc, argv);

    QFile file("Darkeum.qss");


    MainWindow w;
    QSerialPort serial;

    //serial.setPortName("COM19");


    if (file.open(QFile::ReadOnly)) {

        QString styleSheet = QLatin1String(file.readAll());


        a.setStyleSheet(styleSheet);
        file.close();
    }
    w.show();
    return a.exec();
}
