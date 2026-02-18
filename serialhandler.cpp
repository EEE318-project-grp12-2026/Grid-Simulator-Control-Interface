#include "serialhandler.h"
#include <QDebug>

SerialHandler::SerialHandler(QObject *parent)
    : QObject(parent), m_serial(new QSerialPort(this))
{
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialHandler::handleIncomingData);

    // Connect error handler
    connect(m_serial, &QSerialPort::errorOccurred,
            this, &SerialHandler::handleError);
}

bool SerialHandler::openPort(const QString &name, int baudRate)
{
    m_serial->setPortName(name);
    m_serial->setBaudRate(baudRate);
    return m_serial->open(QIODevice::ReadWrite);
}

void SerialHandler::closePort()
{
    if (m_serial->isOpen())
        m_serial->close();

}

bool SerialHandler::isOpen() const
{
    return m_serial->isOpen();
}

void SerialHandler::handleIncomingData()
{
    QByteArray data = m_serial->readLine();
    emit dataReceived(QString(data));
}

void SerialHandler::handleError(QSerialPort::SerialPortError error)
{
    qDebug() << "Serial error:" << error << m_serial->errorString();

    switch (error) {
    case QSerialPort::ResourceError:      // Device unplugged
        m_serial->close();
        emit disconnected();               // ← Notify MainWindow
        break;
    case QSerialPort::DeviceNotFoundError: // Explicit disconnect
        m_serial->close();
        emit disconnected();               // ← Notify MainWindow
        break;

    case QSerialPort::PermissionError:
    case QSerialPort::OpenError:
        emit connectionError("Cannot open port: " + m_serial->errorString());
        break;

    default:
        emit connectionError(m_serial->errorString());
        break;
    }
}
