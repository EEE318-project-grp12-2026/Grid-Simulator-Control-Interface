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
    bool isOpen() const;

signals:
    void dataReceived(const QString &text);
    void disconnected();           // ← New: device unplugged
    void connectionError(const QString &error);  // ← New: other errors

private slots:
    void handleIncomingData();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serial;
};

#endif
