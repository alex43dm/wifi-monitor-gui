#include <unistd.h>

#include <QTextStream>
#include <QDir>

#include "filetail.h"

void FileTail::stop()
{
    quit = false;
}

void FileTail::process()
{
    /*
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 500000000;
    */
    sleep(1);

    while (!quit)
    {
        QDir dir(path, "dump-*.csv");
        dir.setFilter(QDir::Files);
        dir.setSorting(QDir::Time);
        QFileInfoList list = dir.entryInfoList();
        QString fpath;
        for (int i = 0; i < list.size(); ++i)
        {
            QFileInfo fileInfo = list.at(i);
            if (fileInfo.fileName().contains("kismet", Qt::CaseInsensitive))
                continue;
            if (fileInfo.fileName().contains("log", Qt::CaseInsensitive))
                continue;
            fpath = fileInfo.filePath();
            break;
        }

        try
        {
            err.clear();
            FILE *fp = fopen(fpath.toStdString().c_str(), "r");
            if (fp)
            {
                QTextStream s1(fp, QIODevice::ReadOnly);
                emit resultReady(s1.readAll());
                fclose(fp);
            }
            else
            {
                err = "Cann't open file: " + fpath;
            }
        }
        catch (const std::exception &ex)
        {
            err = "Error: " + QString(ex.what());
        }
        sleep(1);

//        nanosleep(&tim , &tim2);
    }
}
