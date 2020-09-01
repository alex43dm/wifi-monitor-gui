#ifndef MACSTAT_H
#define MACSTAT_H

#include <memory>

#include <QObject>

#define MAX_ROW_COUNT_TW2 10
#define DATE_FORMAT "yyyy-MM-dd HH:mm:ss"

struct MACresult
{
    QString mac;
    QString FirstTimeSeen;
    QString LastTimeSeen;
    float powerSum;
    size_t numberInRange;
};

class MACStat : public QObject
{
        Q_OBJECT
        std::shared_ptr<QStringList> buf;
        MACresult *res;
        int kickTimeout;
    public:
        MACStat(std::shared_ptr<QStringList>, const QString &, int);
    public slots:
        void process();
    signals:
        void sendResult(MACresult *);
};

int64_t dateTimeDiff(const QString &firstStr, const QString &lastStr);

#endif // MACSTAT_H
