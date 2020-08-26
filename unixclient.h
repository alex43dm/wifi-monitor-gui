#ifndef UNIXCLIENT_H
#define UNIXCLIENT_H

#include <QString>

class UnixClient
{
    private:
        int sock;
        QString path;
        bool cmd(const QString &);
        const QString cmd_ret(const QString &);
    public:
        UnixClient(const QString &name = "/var/lib/wifi-monitor/sock"): path(name) {};
        bool StartWIFI() {return cmd("START_WIFI");}
        bool StoptWIFI() {return cmd("STOP_WIFI");}
        const QString StatustWIFI() {return cmd_ret("GET_WIFI_STATE");}
        bool StartScan() {return cmd("START_SCAN");}
        bool StopScan() {return cmd("STOP_SCAN");}
        QString err;
};

#endif // UNIXCLIENT_H
