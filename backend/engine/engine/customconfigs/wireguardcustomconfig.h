#ifndef WIREGUARDCUSTOMCONFIG_H
#define WIREGUARDCUSTOMCONFIG_H

#include "icustomconfig.h"

namespace customconfigs {

class WireguardCustomConfig : public ICustomConfig
{
public:
    CUSTOM_CONFIG_TYPE type() const override;
    QString name() const override;      // use for show in the GUI, basically filename without extension
    QString filename() const override;
    QStringList hostnames() const override;      // hostnames/ips

    bool isCorrect() const override;
    QString getErrorForIncorrect() const override;

    static ICustomConfig *makeFromFile(const QString &filepath);

private:
    QString name_;
    QString filename_;

};

} //namespace customconfigs

#endif // WIREGUARDCUSTOMCONFIG_H
