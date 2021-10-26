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

    QString version() const; // 2.x
    QString major() const;
    QString minor() const;
    QString build() const;

    QString fullVersionString() const;     // v2.x.y (Beta) (staging)
    QString semanticVersionString() const; // 2.x.y

private:
    AppVersion();

    QString version_; // 2.x
    ProtoTypes::UpdateChannel buildChannel_;
};

#endif // APPVERSION_H
