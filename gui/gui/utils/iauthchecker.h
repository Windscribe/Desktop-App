#ifndef IAUTHCHECKER_H
#define IAUTHCHECKER_H

#include <QObject>

enum class AuthCheckerError { NO_ERROR, HELPER_ERROR, AUTHENTICATION_ERROR };

class IAuthChecker : public QObject
{
    Q_OBJECT
public:
    explicit IAuthChecker(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IAuthChecker() {}
    virtual AuthCheckerError authenticate() = 0;
};

Q_DECLARE_INTERFACE(IAuthChecker, "IAuthChecker")

#endif // IAUTHCHECKER_H
