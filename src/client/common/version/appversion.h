#pragma once

#include <QString>
#include "windscribe_version.h"
#include "types/enums.h"

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

    // can be switched to staging only for guinea pig builds
#ifdef WINDSCRIBE_IS_GUINEA_PIG
    void switchToStaging() { isStaging_ = true; }
#endif
    bool isStaging() const { return isStaging_; }

private:
    AppVersion();

    QString version_; // 2.x
    bool isStaging_;
    UPDATE_CHANNEL buildChannel_;
};
