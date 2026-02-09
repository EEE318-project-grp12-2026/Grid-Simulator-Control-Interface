#include "mainwindow.h"

#include <QApplication>

#include <qserialport.h>

#include <QSerialPortInfo>

int main(int argc, char *argv[])
{





    QApplication a(argc, argv);
    MainWindow w;
    QSerialPort serial;

    //serial.setPortName("COM19");
    w.show();
    return a.exec();
}
