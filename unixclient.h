#ifndef UNIXCLIENT_H
#define UNIXCLIENT_H

#include <QString>

class UnixClient
{
    private:
        int sock;
        QString path;
        bool cmd(const QString &);
        bool cmd_ret(const QString &);
    public:
        QString iface;
        int wifi_mode;
        int scanning;
        int kick_timeout;
        QString err;

        UnixClient(const QString &name = "/var/lib/wifi-monitor/sock"): path(name) {};
        bool StartWIFI() {return cmd("START_WIFI");}
        bool StoptWIFI() {return cmd("STOP_WIFI");}
        bool Status() {return cmd_ret("GET_STATE");}
        bool StartScan() {return cmd("START_SCAN");}
        bool StopScan() {return cmd("STOP_SCAN");}
        bool SetKickTimeOut(int timeout) {return cmd("SET_KICK_TIMEOUT " + QString::number(timeout));}
};

#endif // UNIXCLIENT_H
