#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btn_ScanPorts_clicked();

    void on_combo_SerPorts_currentTextChanged(const QString &arg1);

    void on_btn_line4disconn_clicked();

    void handleSerialData();

private:
    Ui::MainWindow *ui;

    //QSerialPort *usbserial;
};
#endif // MAINWINDOW_H
