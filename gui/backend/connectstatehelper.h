#ifndef CONNECTSTATEHELPER_H
#define CONNECTSTATEHELPER_H

#include <QObject>
#include "IPC/generated_proto/types.pb.h"

class ConnectStateHelper : public QObject
{
    Q_OBJECT
public:
    explicit ConnectStateHelper(QObject *parent = nullptr);

    void connectClickFromUser();
    void disconnectClickFromUser();
    void setConnectStateFromEngine(const ProtoTypes::ConnectState &connectState);
    bool isDisconnected() const;
    ProtoTypes::ConnectState currentConnectState() const;

signals:
    void connectStateChanged(const ProtoTypes::ConnectState &connectState);

private:
    bool bInternalDisconnecting_;
    bool isDisconnected_;
    ProtoTypes::ConnectState curState_;
};

#endif // CONNECTSTATEHELPER_H
