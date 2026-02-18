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

signals:
    void dataReceived(const QString &text);  // Simple version

private slots:
    void handleIncomingData();

private:
    QSerialPort *m_serial;
};

#endif
