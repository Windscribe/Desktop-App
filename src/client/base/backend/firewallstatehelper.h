#pragma once

#include <QObject>

class FirewallStateHelper : public QObject
{
    Q_OBJECT
public:
    explicit FirewallStateHelper(QObject *parent = nullptr);

    void firewallOnClickFromGUI();
    void firewallOffClickFromGUI();
    void setFirewallStateFromEngine(bool isEnabled);
    bool isEnabled() const;

signals:
    void firewallStateChanged(bool isEnabled);

private:
    bool isEnabled_;
};
