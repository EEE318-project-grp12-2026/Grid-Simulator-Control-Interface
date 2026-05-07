#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QQueue>

class SerialHandler : public QObject
{
    Q_OBJECT
public:
    explicit SerialHandler(QObject *parent = nullptr);
    ~SerialHandler();

    bool openPort(const QString &name, int baudRate);
    void closePort();
    bool isOpen() const;

    // Send methods - distinct names, no ambiguity
    void queueCommand(const QString &text);      // Adds \n, queues async (throttled)
    void queueRaw(const QByteArray &data);       // Raw bytes, queues async (throttled)
    void sendLine(const QString &text);          // Alias for queueCommand
    bool sendNow(const QString &text);           // Blocking with \n (bypasses throttle)
    bool sendNowRaw(const QByteArray &data);     // Blocking raw (bypasses throttle)

    // Diagnostics
    int  pendingCount() const { return m_writeQueue.size(); }
    void clearPending();

signals:
    void lineReceived(const QString &line);
    void rawDataReceived(const QByteArray &data);
    void disconnected();
    void connectionError(const QString &error);
    void sendError(const QString &error);

private slots:
    void onReadyRead();
    void onBytesWritten(qint64 bytes);
    void onError(QSerialPort::SerialPortError error);
    void processQueue();
    void onWriteTimeout();
    void onCooldownExpired();

private:
    struct QueueItem {
        QByteArray data;
    };

    QSerialPort *m_serial;
    QByteArray m_readBuffer;
    QQueue<QueueItem> m_writeQueue;
    bool m_writeInProgress = false;
    bool m_inCooldown      = false;

    QTimer *m_queueTimer;
    QTimer *m_writeTimeout;
    QTimer *m_cooldownTimer;

    // Throttle parameters — tuned so MCU doesn't drop commands
    static constexpr int QUEUE_INTERVAL_MS    = 50;   // queue drain tick rate
    static constexpr int WRITE_TIMEOUT_MS     = 500;  // give up on stuck write
    static constexpr int INTER_COMMAND_MS     = 50;   // mandatory gap between sends
    static constexpr int MAX_QUEUE_DEPTH      = 64;   // drop oldest beyond this
};

#endif
