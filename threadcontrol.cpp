#include "threadcontrol.h"
#include "mainwindow.h"

ThreadControl::ThreadControl(QObject *parent) : QObject(parent)
{
    worker = new FileTail;
    worker->moveToThread(&thr);

    connect(&thr, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &ThreadControl::operate, worker, &FileTail::Read);
    connect(worker, &FileTail::resultReady, this, &Ui::MainWindow::handleResults);

    thr.start();
}

ThreadControl::~ThreadControl()
{
    worker->quit = true;

    thr.quit();
    thr.wait();

    delete  worker;
}

