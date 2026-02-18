#include "serialhandler.h"
#include <QDebug>
//#include "mainwindow.h"

SerialHandler::SerialHandler(QObject *parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
{
    // Connect the "button press" equivalent for serial data
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialHandler::handleIncomingData);



    connect(m_serial, &QSerialPort::errorOccurred,
            this, &SerialHandler::handleError);
}

SerialHandler::~SerialHandler()
{
    if (m_serial->isOpen())
        m_serial->close();
}

bool SerialHandler::openPort(const QString &name, int baudRate)
{
    m_serial->setPortName(name);
    m_serial->setBaudRate(baudRate);
    return m_serial->open(QIODevice::ReadWrite);
}

void __attribute__((weak)) SerialHandler::handleIncomingData()
{

    QByteArray data = m_serial->readAll();
    qDebug() << "Data arrived:" << data;
    emit dataReceived(QString(data));



}

void SerialHandler::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
        qDebug() << "Serial error:" << m_serial->errorString();
}
