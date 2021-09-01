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
    if (buildChannel_ == ProtoTypes::UPDATE_CHANNEL_BETA)
    {
        return "v" + version() + " Build " + build() + " (Beta) (Staging)";
    }
    else if (buildChannel_ == ProtoTypes::UPDATE_CHANNEL_GUINEA_PIG)
    {
        return "v" + version() + " Build " + build() + " (Guinea Pig) (Staging)";
    }
    else
    {
        return "v" + version() + " Build " + build() + " (Staging)";
    }
#else
    if (buildChannel_ == ProtoTypes::UPDATE_CHANNEL_BETA)
    {
        return "v" + version() + " Build " + build() + " (Beta)";
    }
    else if (buildChannel_ == ProtoTypes::UPDATE_CHANNEL_GUINEA_PIG)
    {
        return "v" + version() + " Build " + build() + " (Guinea Pig)";
    }
    else
    {
        return "v" + version() + " Build " + build();
    }
#endif
}

AppVersion::AppVersion() :
    version_(QString::asprintf("%d.%d", WINDSCRIBE_MAJOR_VERSION, WINDSCRIBE_MINOR_VERSION)),
    build_(QString::asprintf("%d", WINDSCRIBE_BUILD_VERSION)),
#ifdef WINDSCRIBE_IS_BETA
    buildChannel_(ProtoTypes::UPDATE_CHANNEL_BETA)
#elif defined WINDSCRIBE_IS_GUINEA_PIG
    buildChannel_(ProtoTypes::UPDATE_CHANNEL_GUINEA_PIG)
#else
    buildChannel_(ProtoTypes::UPDATE_CHANNEL_RELEASE)
#endif
{
}
