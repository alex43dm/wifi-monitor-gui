#ifndef FILETAIL_H
#define FILETAIL_H

#include <QObject>
#include <QString>

class FileTail: public QObject
{
        Q_OBJECT
    public:
        QString path;
        QString err;
        bool quit;

    public slots:
        void process();
        void stop();

    signals:
        void resultReady(const QString &);
        void finished();
};

#endif // FILETAIL_H
