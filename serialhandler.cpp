#include "serialhandler.h"
#include <QDebug>

SerialHandler::SerialHandler(QObject *parent)
    : QObject(parent), m_serial(new QSerialPort(this))
{
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialHandler::handleIncomingData);
}

bool SerialHandler::openPort(const QString &name, int baudRate)
{
    m_serial->setPortName(name);
    m_serial->setBaudRate(baudRate);
    bool ok = m_serial->open(QIODevice::ReadWrite);
    qDebug() << "Opening port:" << name << "Result:" << ok;
    return ok;
}

void SerialHandler::handleIncomingData()
{
    QByteArray data = m_serial->readAll();
    qDebug() << "Serial data received:" << data;
    emit dataReceived(QString(data));
}
