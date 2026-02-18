#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>



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
    ui->combo_SerPorts->clear();
    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()){

#ifdef Q_OS_LINUX

        // filter out unwanted ports that have nothing to do with USB or external ports. (LINUX)

        if (port.portName().contains("USB") or
            port.portName().contains("ACM")){

            ui->combo_SerPorts->addItem(port.portName());

        }

#elif  defined(Q_OS_WIN)

        // filter out unwanted ports that have nothing to do with USB or external ports. (WINDOWS)


        if(port.portName().contains("COM")){
            //add port to list

            ui->combo_SerPorts->addItem(port.portName());


        }

#else
#error no host OS defined, see Q_OS_WIN, Q_OS_LINUX or other relevant macro




#endif


    }


}




void MainWindow::on_combo_SerPorts_currentTextChanged(const QString &arg1)
{
    if(!arg1.isEmpty()){

        m_serial->disconnect();


        connect(m_serial, &SerialHandler::dataReceived,
                this, &MainWindow::updateStatusLabel);




        if (!m_serial->openPort(arg1, 115200)) {
            qDebug() << "Failed to open serial port";















        __SER_STAT = STAT_CON;






    }


\


}

}

void MainWindow::on_btn_line4disconn_clicked()
{
    ui->btn_line_upgrade->setText("test");

    ui->listWidget->addItem("Disconnected BusBar B");
}













void MainWindow::on_pushButton_clicked()
{
    qDebug()<<usbserial.readLine();

}



void MainWindow::updateStatusLabel(const QString &text)
{
    //ui->statusLabel->setText("Received: " + text);
    // Any UI element: ui->lineEdit, ui->textEdit, ui->progressBar, etc.

    qDebug()<<"this other fucking bit is running";


    ui->listWidget->addItem(text);
    ui->listWidget->scrollToBottom();

    qDebug()<<text<<"\n";
}


