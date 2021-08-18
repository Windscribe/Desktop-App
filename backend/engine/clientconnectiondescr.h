#ifndef CLIENTCONNECTIONDESCR_H
#define CLIENTCONNECTIONDESCR_H

#include <QString>

class ClientConnectionDescr
{
public:
    ClientConnectionDescr();

public:
    bool bClientAuthReceived_;          // is the client made authorization
    unsigned int protocolVersion_;
    unsigned int clientId_;
    unsigned long pid_;
    QString name_;
    qint64 latestCommandTimeMs_;
};

#endif // CLIENTCONNECTIONDESCR_H
