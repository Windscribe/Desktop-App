#ifndef APPVERSION_H
#define APPVERSION_H

#include <QString>
#include "utils/protobuf_includes.h"

class AppVersion
{
public:
    static AppVersion &instance()
    {
        static AppVersion s;
        return s;
    }

    QString version() const;
    QString build() const;
    QString getFullString() const;

private:
    AppVersion();

    QString version_;
    QString build_;

    ProtoTypes::UpdateChannel buildChannel_;
};

#endif // APPVERSION_H
