#ifndef AUTHCHECKER_MAC_H
#define AUTHCHECKER_MAC_H

#include <QObject>
#include "iauthchecker.h"

class AuthChecker_mac : public IAuthChecker, QObject
{
    Q_OBJECT
public:
    explicit AuthChecker_mac(QObject *parent = 0);
    ~AuthChecker_mac();

    bool authenticate() override;
};

#endif
