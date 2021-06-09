#ifndef AUTHCHECKER_MAC_H
#define AUTHCHECKER_MAC_H

#include <QObject>

class AuthChecker_mac : public QObject
{
    Q_OBJECT
public:
    explicit AuthChecker_mac(QObject *parent = 0);
    ~AuthChecker_mac();

    bool authenticate();
    void deauthenticate();
    bool isAuthenticated();

};

#endif
