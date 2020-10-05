#ifndef OVPNCUSTOMCONFIG_H
#define OVPNCUSTOMCONFIG_H

#include "icustomconfig.h"

namespace customconfigs {

class OvpnCustomConfig : public ICustomConfig
{
public:
    CUSTOM_CONFIG_TYPE type() const override;
    QString name() const override;      // use for show in the GUI, basically filename without extension
    QString filename() const override;  // filename (without full path)
    QStringList hostnames() const override;      // hostnames/ips

    bool isCorrect() const override;
    QString getErrorForIncorrect() const override;

    static ICustomConfig *makeFromFile(const QString &filepath);

private:
    QString name_;
    QString filename_;
    QString filepath_;

    void process();

};

} //namespace customconfigs

#endif // OVPNCUSTOMCONFIG_H
