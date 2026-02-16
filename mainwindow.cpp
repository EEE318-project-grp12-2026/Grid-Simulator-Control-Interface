#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>
QSerialPort usbserial;

#define STAT_NOTCON 0
#define STAT_CON    1
#define STAT_ERR    2
int __SER_STAT = STAT_NOTCON;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

        if (1 or port.portName().contains("USB") or
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
    if(arg1 == "s"){
        __asm("nop");
    }


    usbserial.setPortName("tty");
    usbserial.setBaudRate(QSerialPort::Baud115200);
    qDebug()<<"connected to: "<<arg1<<"\n";
    ui->lbl_connStat->setText("Connected to port: " + arg1);

    __SER_STAT = STAT_CON;


\


}


void MainWindow::on_btn_line4disconn_clicked()
{
    ui->btn_line_upgrade->setText("test");

    ui->listWidget->addItem("Disconnected BusBar B");
}




void MainWindow::handleSerialData() {

    if(STAT_CON == __SER_STAT){
        //read data

        __asm("nop");
        QByteArray data = usbserial.readLine(); //use readline to parse line by line. use AT CMD structure.

        // do some shit with the data - figure out what the  fuck it is and shove it into the gui somewhere

    }

}






