#ifndef APPVERSION_H
#define APPVERSION_H

#include <QString>
#include "ipc/generated_proto/types.pb.h"

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

    int major_;
    int minor_;
    int buildInt_;

    ProtoTypes::UpdateChannel buildChannel_;
};

#endif // APPVERSION_H
