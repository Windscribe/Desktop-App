#pragma once

#include "iauthchecker.h"

class AuthChecker_win : public IAuthChecker
{
    Q_OBJECT
    Q_INTERFACES(IAuthChecker)
public:
    explicit AuthChecker_win(QObject *parent = nullptr);
    ~AuthChecker_win() override {}
    AuthCheckerError authenticate() override;
};
