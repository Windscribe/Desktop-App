#ifndef OPENLOCATIONSHANDER_MAC_H
#define OPENLOCATIONSHANDER_MAC_H

#include <QObject>

class OpenLocationsHandler_mac : public QObject
{
    Q_OBJECT
public:
    explicit OpenLocationsHandler_mac(QObject *parent = 0);

    void emitReceivedOpenLocationsMessage();
    void unsuspendObservers();

signals:
    void receivedOpenLocationsMessage();
};

#endif // OPENLOCATIONSHANDER_MAC_H
