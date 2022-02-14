#ifndef AUTHCHECKER_MAC_H
#define AUTHCHECKER_MAC_H

#include "iauthchecker.h"

class AuthChecker_mac : public IAuthChecker
{
    Q_OBJECT
    Q_INTERFACES(IAuthChecker)
public:
    explicit AuthChecker_mac(QObject *parent = nullptr);
    ~AuthChecker_mac() override;
    AuthCheckerError authenticate() override;
};

#endif
