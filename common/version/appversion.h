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

    void switchToStaging() { isStaging_ = true; }
    bool isStaging() const { return isStaging_; }

private:
    AppVersion();

    QString version_; // 2.x
    bool isStaging_;
    ProtoTypes::UpdateChannel buildChannel_;
};

#endif // APPVERSION_H
