#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <QObject>
#include <QSerialPort>

class SerialHandler : public QObject
{
    Q_OBJECT
public:
    explicit SerialHandler(QObject *parent = nullptr);
    bool openPort(const QString &name, int baudRate);
    void closePort();

    // Send methods
    bool sendLine(const QString &text);      // Adds \n automatically
    bool sendData(const QByteArray &data); // Raw bytes
    bool sendString(const QString &text);  // No newline added

signals:
    void lineReceived(const QString &line);
    void disconnected();
    void connectionError(const QString &error);

private slots:
    void handleIncomingData();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serial;
    QByteArray m_buffer;
};

#endif
