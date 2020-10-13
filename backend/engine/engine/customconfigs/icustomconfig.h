#ifndef ICUSTOMCONFIG_H
#define ICUSTOMCONFIG_H

#include <QStringList>
#include "customconfigtype.h"

namespace customconfigs {

// Base interface for custom config
class ICustomConfig
{
public:
    virtual ~ICustomConfig() {}

    virtual CUSTOM_CONFIG_TYPE type() const = 0;
    virtual QString name() const = 0;       // use for show in the GUI, basically filename without extension
    virtual QString filename() const = 0;   // filename (without full path), can be used as unique id of config
    virtual QStringList hostnames() const = 0;      // hostnames/ips -> they are used for ping and measure latency

    virtual bool isCorrect() const = 0;
    virtual QString getErrorForIncorrect() const = 0;
};

} //namespace customconfigs

#endif // ICUSTOMCONFIG_H
