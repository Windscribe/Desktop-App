#pragma once

#include <QVector>
#include "icustomconfig.h"

namespace customconfigs {

struct RemoteCommandLine
{
    QString hostname;               // ip or hostname
    QString originalRemoteCommand;
    uint port;              // 0 if not set
    QString protocol;       // empty if not set
};

class OvpnCustomConfig : public ICustomConfig
{
public:
    OvpnCustomConfig() = default;
    CUSTOM_CONFIG_TYPE type() const override;
    QString name() const override;      // use for show in the GUI, basically filename without extension
    QString nick() const override;      // use for show in the GUI, host to connect to
    QString filename() const override;  // filename (without full path). It is used as an identifier for the LocationID
    QStringList hostnames() const override;      // hostnames/ips
    bool isAllowFirewallAfterConnection() const override;

    bool isCorrect() const override;
    QString getErrorForIncorrect() const override;

    static ICustomConfig *makeFromFile(const QString &filepath);

    QVector<RemoteCommandLine> remotes() const;
    uint globalPort() const;
    QString globalProtocol() const;
    QString getOvpnData() const;

private:
    bool isCorrect_ = false;
    QString errMessage_;

    QString name_;
    QString nick_;
    QString filename_;
    QString filepath_;

    QString ovpnData_;  // ovpn file without "remote" commands. They are all cut out and converted to a list of structures "RemoteItem"
    QVector<RemoteCommandLine> remotes_;

    uint globalPort_ = 0;       // 0 if not set
    QString globalProtocol_;    // empty if not set
    bool isAllowFirewallAfterConnection_ = true;


    void process();

};

} //namespace customconfigs
