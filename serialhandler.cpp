#include "serialhandler.h"
#include <QDebug>

SerialHandler::SerialHandler(QObject *parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
    , m_queueTimer(new QTimer(this))
    , m_writeTimeout(new QTimer(this))
    , m_cooldownTimer(new QTimer(this))
{
    connect(m_serial, &QSerialPort::readyRead, this, &SerialHandler::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialHandler::onError);
    connect(m_serial, &QSerialPort::bytesWritten, this, &SerialHandler::onBytesWritten);

    m_queueTimer->setInterval(QUEUE_INTERVAL_MS);
    m_queueTimer->setSingleShot(false);
    connect(m_queueTimer, &QTimer::timeout, this, &SerialHandler::processQueue);

    m_writeTimeout->setInterval(WRITE_TIMEOUT_MS);
    m_writeTimeout->setSingleShot(true);
    connect(m_writeTimeout, &QTimer::timeout, this, &SerialHandler::onWriteTimeout);

    m_cooldownTimer->setInterval(INTER_COMMAND_MS);
    m_cooldownTimer->setSingleShot(true);
    connect(m_cooldownTimer, &QTimer::timeout, this, &SerialHandler::onCooldownExpired);
}

SerialHandler::~SerialHandler()
{
    closePort();
}

bool SerialHandler::openPort(const QString &name, int baudRate)
{
    if (m_serial->isOpen()) closePort();

    m_serial->setPortName(name);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    m_readBuffer.clear();
    m_writeQueue.clear();
    m_writeInProgress = false;
    m_inCooldown = false;

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit connectionError("Failed to open " + name);
        return false;
    }

    m_queueTimer->start();
    return true;
}

void SerialHandler::closePort()
{
    m_queueTimer->stop();
    m_writeTimeout->stop();
    m_cooldownTimer->stop();
    m_writeQueue.clear();
    m_writeInProgress = false;
    m_inCooldown = false;
    if (m_serial->isOpen()) m_serial->close();
}

bool SerialHandler::isOpen() const
{
    return m_serial->isOpen();
}

void SerialHandler::clearPending()
{
    m_writeQueue.clear();
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

void SerialHandler::queueCommand(const QString &text)
{
    QueueItem item;
    item.data = text.toUtf8() + '\n';

    // Cap queue depth so a stuck MCU doesn't cause unbounded growth
    while (m_writeQueue.size() >= MAX_QUEUE_DEPTH) {
        m_writeQueue.dequeue();
    }
    m_writeQueue.enqueue(item);

    // Try to start draining now if we're idle
    if (!m_writeInProgress && !m_inCooldown) {
        processQueue();
    }
}

void SerialHandler::queueRaw(const QByteArray &data)
{
    QueueItem item;
    item.data = data;

    while (m_writeQueue.size() >= MAX_QUEUE_DEPTH) {
        m_writeQueue.dequeue();
    }
    m_writeQueue.enqueue(item);

    if (!m_writeInProgress && !m_inCooldown) {
        processQueue();
    }
}

void SerialHandler::sendLine(const QString &text)
{
    queueCommand(text);
}

bool SerialHandler::sendNow(const QString &text)
{
    return sendNowRaw(text.toUtf8() + '\n');
}

bool SerialHandler::sendNowRaw(const QByteArray &data)
{
    if (!m_serial->isOpen()) return false;

    qint64 written = m_serial->write(data);
    if (written < 0) {
        emit sendError("Immediate write failed");
        return false;
    }
    bool flushed = m_serial->flush();
    return flushed && (written == data.size());
}

/* ------------------------------------------------------------------ */
/*  Queue processor — sends one command, then cools down              */
/* ------------------------------------------------------------------ */

void SerialHandler::processQueue()
{
    if (m_writeInProgress) return;
    if (m_inCooldown)      return;
    if (m_writeQueue.isEmpty()) return;
    if (!m_serial->isOpen()) return;

    QueueItem item = m_writeQueue.dequeue();
    QByteArray payload = item.data;

    qint64 written = m_serial->write(payload);
    if (written < 0 || written != payload.size()) {
        emit sendError("Partial write detected");
        m_writeInProgress = false;
        return;
    }

    m_serial->flush();
    m_writeInProgress = true;
    m_writeTimeout->start();
}

void SerialHandler::onBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes)
    m_writeTimeout->stop();
    m_writeInProgress = false;

    // Start the inter-command cooldown — next send waits at least
    // INTER_COMMAND_MS so the MCU has time to parse and dispatch.
    m_inCooldown = true;
    m_cooldownTimer->start();
}

void SerialHandler::onCooldownExpired()
{
    m_inCooldown = false;
    // Drain next queued command if any
    if (!m_writeQueue.isEmpty() && !m_writeInProgress) {
        processQueue();
    }
}

void SerialHandler::onWriteTimeout()
{
    qDebug() << "Write timeout - resetting write state";
    m_writeInProgress = false;
    m_inCooldown = false;
}

/* ------------------------------------------------------------------ */
/*  Read handling                                                     */
/* ------------------------------------------------------------------ */

void SerialHandler::onReadyRead()
{
    m_readBuffer.append(m_serial->readAll());
    emit rawDataReceived(m_readBuffer);

    int nl = m_readBuffer.indexOf('\n');
    while (nl != -1) {
        QByteArray line = m_readBuffer.left(nl);
        if (line.endsWith('\r')) line.chop(1);

        m_readBuffer.remove(0, nl + 1);
        emit lineReceived(QString::fromUtf8(line));

        nl = m_readBuffer.indexOf('\n');
    }

    if (m_readBuffer.size() > 4096) m_readBuffer.clear();
}

void SerialHandler::onError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;

    if (error == QSerialPort::ResourceError) {
        closePort();
        emit disconnected();
    } else {
        emit connectionError(m_serial->errorString());
    }
}
