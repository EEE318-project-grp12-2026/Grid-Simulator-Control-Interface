#include <QSerialPort>
#include <QObject>
#include <QDebug>

class SerialHandler : public QObject {
    Q_OBJECT
public:
    SerialHandler(QObject *parent = nullptr) : QObject(parent) {
        serial = new QSerialPort(this);
        serial->setPortName("COM3");  // or /dev/ttyUSB0 on Linux
        serial->setBaudRate(QSerialPort::Baud9600);

        // This is the key connection - like button->clicked
        connect(serial, &QSerialPort::readyRead,
                this, &SerialHandler::handleIncomingData);

        serial->open(QIODevice::ReadWrite);
    }

private slots:
    void handleIncomingData() {
        QByteArray data = serial->readAll();
        // Execute your code here - runs automatically when data arrives
        processData(data);
    }

private:
    QSerialPort *serial;

    void processData(const QByteArray &data) {
        qDebug() << "Received:" << data;
        // Your application logic here
    }
};
