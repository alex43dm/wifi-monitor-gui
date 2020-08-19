#ifndef THREADCONTROL_H
#define THREADCONTROL_H

#include <QObject>
#include <QThread>

#include "filetail.h"

class ThreadControl : public QObject
{
        Q_OBJECT
        QThread thr;
        FileTail *worker;
    public:
        explicit ThreadControl(QObject *parent = nullptr);
        ~ThreadControl();
    public slots:
        void handleResults(const QString &);
    signals:
        void operate(const QString &);
};

#endif // THREADCONTROL_H
