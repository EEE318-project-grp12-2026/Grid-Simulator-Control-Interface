#include "mainwindow.h"
#include <QApplication>
#include "DarkeumStyle.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(StyleSheet);

    MainWindow w;
    w.show();

    return a.exec();
}
