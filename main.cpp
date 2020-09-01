#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/new/prefix1/wifi.jpg"));
    MainWindow w;
    w.show();
    return a.exec();
}
