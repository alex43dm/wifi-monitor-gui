#include <QDateTime>

#include "macstat.h"

int64_t dateTimeDiff(const QString &firstStr, const QString &lastStr)
{
    return QDateTime().fromString(firstStr.trimmed(),
                                  DATE_FORMAT).secsTo(QDateTime().fromString(lastStr.trimmed(), DATE_FORMAT));
}

MACStat::MACStat(std::shared_ptr<QStringList> s, const QString &m, int kt)
{
    buf = s;
    res = new MACresult();
    res->mac = m;
    kickTimeout = kt;
}

void MACStat::process()
{
    int cnt = 0;
    int powerSum = 0;
    for (const QString &line : *buf)
    {
        if (!line.contains(res->mac))
            continue;

        QStringList list = line.split(QLatin1Char(','));

        cnt++;
        powerSum += list.at(4).toInt();

        if (res->FirstTimeSeen.size() == 0)
        {
            res->FirstTimeSeen = list.at(0);
        }

        if (dateTimeDiff(res->LastTimeSeen, list.at(0)) > kickTimeout)
        {
            res->numberInRange++;
        }

        res->LastTimeSeen = list.at(0);
    }
    if (cnt == 0) cnt = 1;
    res->powerSum = abs(powerSum) / cnt;

    emit sendResult(res);
}
