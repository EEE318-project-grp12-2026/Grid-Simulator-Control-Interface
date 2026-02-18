#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <QObject>
#include <QSerialPort>

class SerialHandler : public QObject
{
    Q_OBJECT
public:
    explicit SerialHandler(QObject *parent = nullptr);
    ~SerialHandler();

    bool openPort(const QString &name, int baudRate);
   // void handleIncomingData();




signals:
    void dataReceived(const QString &text);      // for text


    void byteArrayReceived(const QByteArray &data);  // for raw bytes


    void lineReceived(const QString &line);      // for line-terminated data


private slots:

    void handleError(QSerialPort::SerialPortError error);

    void handleIncomingData();

private:
    QSerialPort *m_serial;

    QByteArray m_buffer;
};

#endif // SERIALHANDLER_H
