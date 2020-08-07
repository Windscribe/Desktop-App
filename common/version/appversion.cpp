#include "appversion.h"
#include "windscribe_version.h"

#include <QStringList>

QString AppVersion::version() const
{
    return version_;
}

QString AppVersion::build() const
{
    return build_;
}

QString AppVersion::getFullString() const
{
#ifdef WINDSCRIBE_IS_STAGING
    if (isBeta_)
    {
        return "v" + version() + " Build " + build() + " (Beta) (Staging)";
    }
    else
    {
        return "v" + version() + " Build " + build() + " (Staging)";
    }
#else
    if (isBeta_)
    {
        return "v" + version() + " Build " + build() + " (Beta)";
    }
    else
    {
        return "v" + version() + " Build " + build();
    }
#endif
}

AppVersion::AppVersion()
{
    version_.asprintf("%d.%02d", WINDSCRIBE_MAJOR_VERSION, WINDSCRIBE_MINOR_VERSION);
    major_ = WINDSCRIBE_MAJOR_VERSION;
    minor_ = WINDSCRIBE_MINOR_VERSION;
    build_.asprintf("%d", WINDSCRIBE_BUILD_VERSION);
    buildInt_ = WINDSCRIBE_BUILD_VERSION;
#ifdef WINDSCRIBE_IS_BETA
    isBeta_ = true;
#else
    isBeta_ = false;
#endif
}
