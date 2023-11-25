#pragma once

#include <QFuture>
#include <QHostInfo>
#include <QObject>

class ExitHandler_mac : public QObject
{
    Q_OBJECT
public:
    explicit ExitHandler_mac(QObject *parent = 0);
    virtual ~ExitHandler_mac();

    bool isExitWithRestart() { return bExitWithRestart_; }
    void setExitWithRestart() { bExitWithRestart_ = true; }

    static ExitHandler_mac *this_;

signals:
    void shouldTerminate();

private:
    bool bExitWithRestart_;
};
