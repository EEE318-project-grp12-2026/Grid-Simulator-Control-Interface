#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>



#include "serialhandler.h"
QSerialPort usbserial;

#define STAT_NOTCON 0
#define STAT_CON    1
#define STAT_ERR    2
int __SER_STAT = STAT_NOTCON;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serial(new SerialHandler(this))
{
    ui->setupUi(this);


    connect(m_serial, &SerialHandler::disconnected,
            this, &MainWindow::onDisconnected);

    connect(m_serial, &SerialHandler::connectionError,
            this, &MainWindow::onConnectionError);



  //  qDebug() << "Signal/slot connected:" << connected;



    // Open the port
   // if (!m_serial->openPort("ttyACM0", 115200)) {
     //   qDebug() << "Failed to open serial port";
//}
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btn_ScanPorts_clicked()
{

    bool PortsAvailable = false;
    ui->combo_SerPorts->clear();

#ifdef Q_OS_LINUX
    QString errMsg = "Check that the device is connected and make sure the application and user has the correct permissions";


    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()){


        // filter out unwanted ports that have nothing to do with USB or external ports. (LINUX)

        if (port.portName().contains("USB") or
            port.portName().contains("ACM")){

            ui->combo_SerPorts->addItem(port.portName());

            PortsAvailable = true;

        }

#elif  defined(Q_OS_WIN)

    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()){

        QString errMsg = "Check that the device is plugged in";


        // filter out unwanted ports that have nothing to do with USB or external ports. (WINDOWS)


        if(port.portName().contains("COM")){
            //add port to list

            ui->combo_SerPorts->addItem(port.portName());


        }

#else
#error no host OS defined, see Q_OS_WIN, Q_OS_LINUX or other relevant macro




#endif


    }

    if(!PortsAvailable){
        QMessageBox::warning(this, "No Devices Found", errMsg, "ok");
    }


}




void MainWindow::on_combo_SerPorts_currentTextChanged(const QString &arg1)
{
    __asm("nop");

}

void MainWindow::on_btn_line4disconn_clicked()
{
    ui->btn_line_upgrade->setText("test");

    ui->listWidget->addItem("Disconnected BusBar B");
}













void MainWindow::on_pushButton_clicked()
{
//    qDebug()<<usbserial.readLine();


m_serial->sendLine("c");




}

bool OVERFLOW_DEBUG = false;
// ON DATA RECIEVED VIA SERIAL - NEWLINES ONLY
void MainWindow::onSerialDataSignalRecieved(const QString &text)
{
    //ui->statusLabel->setText("Received: " + text);
    // Any UI element: ui->lineEdit, ui->textEdit, ui->progressBar, etc.




    ui->listWidget->addItem("DATA REDIEVED: " + text);


    if(ui->listWidget->count() > 200){

        ui->listWidget->takeItem(0);

        qDebug()<<"Limit Reached. DEL 1st item";
       // OVERFLOW_DEBUG = true;

        if (!OVERFLOW_DEBUG){

            OVERFLOW_DEBUG = true;
            ui->lblOVERFLOW->setText("LIMIT REACHED! DATA OVERFLOW!");
        }
    }
    ui->listWidget->scrollToBottom();

    qDebug()<<text<<"\n";
}




void MainWindow::onDisconnected()
{

    qDebug()<< "successful disconnect handler call";
    m_isConnected = false;


    // Clean up the data connection
    disconnect(m_serial, &SerialHandler::lineReceived,
               this, &MainWindow::onSerialDataSignalRecieved);
}


void MainWindow::onConnectionError(const QString &error)
{
    qDebug()<<"disconnected: " + error;


}

void MainWindow::on_btnConnectSerial_clicked()
{
    QString arg1 = ui->combo_SerPorts->currentText();

    if(!arg1.isEmpty() and !m_isConnected){

        m_serial->disconnect();


        connect(m_serial, &SerialHandler::lineReceived,
                this, &MainWindow::onSerialDataSignalRecieved);




        if (!m_serial->openPort(arg1, 115200)) {
            qDebug() << "Failed to open serial port: ";
            // user likely has disconnected the device. tell them

            ui->combo_SerPorts->clear();
            if (1 ==QMessageBox::critical(this, "Serial Fault!", "Have you disconnected the device?",  "yes", "no")){

                QMessageBox::critical(this, "RESOURCE BUSY!",
                                      "Resource used by another application. if there is no other applications using serial, you should reboot your PC. ",
                                      "OK");

            }else{

                if(QMessageBox::information(this, "Reconnect the device",
                                             "The device should be plugged in again. use the\"scan ports\" button to search for devices again",
                                             "ok")){


                }
            }





            __SER_STAT = STAT_ERR;

        }else{
            m_isConnected = true;
            ui->btnConnectSerial->setText("Disconnect Serial");

            this->setWindowTitle("Connected to a serial device");


        }




    }else if(m_isConnected){


        m_serial->closePort();
        qDebug()<<"cloded port";

        m_isConnected = false;
        ui->btnConnectSerial->setText("Connect Serial");

         this->setWindowTitle("Device Disconnected");
    }else if (arg1.isEmpty()){

        QMessageBox::warning(this, "Warning",
                             "There is no devices known. Search for a Device using the Scan Ports button", "OK");
    }
}

