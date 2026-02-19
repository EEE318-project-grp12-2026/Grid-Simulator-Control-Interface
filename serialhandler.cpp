#include "serialhandler.h"
#include <QDebug>

SerialHandler::SerialHandler(QObject *parent)
    : QObject(parent), m_serial(new QSerialPort(this))
{
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialHandler::handleIncomingData);
    connect(m_serial, &QSerialPort::errorOccurred,
            this, &SerialHandler::handleError);
}

bool SerialHandler::openPort(const QString &name, int baudRate)
{
    m_serial->setPortName(name);
    m_serial->setBaudRate(baudRate);
    m_buffer.clear();
    return m_serial->open(QIODevice::ReadWrite);
}

void SerialHandler::closePort()
{
    if (m_serial->isOpen())
        m_serial->close();
}

// Send with automatic newline
bool SerialHandler::sendLine(const QString &text)
{
    if (!m_serial->isOpen()) return false;

    QByteArray data = text.toUtf8() + '\n';
    qint64 written = m_serial->write(data);
    m_serial->flush();  // Ensure immediate send
    return written == data.size();
}

// Send raw string without newline
bool SerialHandler::sendString(const QString &text)
{
    if (!m_serial->isOpen()) return false;

    QByteArray data = text.toUtf8();
    qint64 written = m_serial->write(data);
    m_serial->flush();
    return written == data.size();
}

// Send raw bytes
bool SerialHandler::sendData(const QByteArray &data)
{
    if (!m_serial->isOpen()) return false;

    qint64 written = m_serial->write(data);
    m_serial->flush();
    return written == data.size();
}

void SerialHandler::handleIncomingData()
{
    m_buffer.append(m_serial->readAll());

    int newlineIndex = m_buffer.indexOf('\n');
    while (newlineIndex != -1) {
        QByteArray lineData = m_buffer.left(newlineIndex);
        if (lineData.endsWith('\r')) lineData.chop(1);

        m_buffer.remove(0, newlineIndex + 1);
        emit lineReceived(QString::fromUtf8(lineData));

        newlineIndex = m_buffer.indexOf('\n');
    }

    if (m_buffer.size() > 1000) m_buffer.clear();
}

void SerialHandler::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError ||
        error == QSerialPort::UnsupportedOperationError) {
        m_serial->close();
        m_buffer.clear();
        emit disconnected();
    } else if (error != QSerialPort::NoError) {
        emit connectionError(m_serial->errorString());
    }
}
