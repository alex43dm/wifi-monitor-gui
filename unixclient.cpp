#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "unixclient.h"

bool UnixClient::cmd(const QString &command)
{
    struct sockaddr_un address;

    err.clear();

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        err =  "Failed create connect socket to " + path;
        return true;
    }

    memset(&address, 0x00, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, path.toStdString().c_str(), path.size());

    if (connect(sock, (struct sockaddr *)&address, sizeof(address)))
    {
        err =  "Failed connect socket to " + path + " errno: " + errno + " : " + strerror(errno);
        close(sock);
        return true;
    }

    if (send(sock, command.toStdString().c_str(), command.size(), 0) < 0)
    {
        err =  "Error writing to socket " + path + " errno: " + errno + " : " + strerror(errno);
        close(sock);
        return true;
    }

    close(sock);
    return false;
}
