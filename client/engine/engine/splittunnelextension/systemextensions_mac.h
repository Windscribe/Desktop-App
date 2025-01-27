#pragma once

#include <QObject>

class SystemExtensions_mac : public QObject {
    Q_OBJECT
public:
    static SystemExtensions_mac* instance();

    enum SystemExtensionState {
        Active,
        Inactive,
        PendingUserApproval,
        Unknown
    };

    bool setAppProxySystemExtensionActive(bool isActive);
    SystemExtensionState getSystemExtensionState();

signals:
    void stateChanged(SystemExtensionState newState);

public slots:
    void onExtensionStateChanged(SystemExtensionState newState);

private:
    SystemExtensions_mac();
    ~SystemExtensions_mac();
    
    // Delete copy constructor and assignment operator
    SystemExtensions_mac(const SystemExtensions_mac&) = delete;
    SystemExtensions_mac& operator=(const SystemExtensions_mac&) = delete;

    static SystemExtensions_mac* instance_;
};
