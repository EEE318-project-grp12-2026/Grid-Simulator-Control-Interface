#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>

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
    if(arg1 == "s"){
        __asm("nop");
    }


}

