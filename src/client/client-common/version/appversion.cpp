#include "appversion.h"

#include <QStringList>

QString AppVersion::version() const
{
    return version_;
}

QString AppVersion::major() const
{
    return QString::number(WS_VERSION_MAJOR);
}

QString AppVersion::minor() const
{
    return QString::number(WS_VERSION_MINOR);
}

QString AppVersion::build() const
{
    return QString::number(WS_VERSION_BUILD);
}

QString AppVersion::fullVersionString() const
{
    if (isStaging_)
    {
#ifdef USE_BUILD_ID
        return "v" + semanticVersionString() + " (#" + USE_BUILD_ID + ") (Staging)";
#else
        if (buildChannel_ == UPDATE_CHANNEL_BETA)
        {
            return "v" + semanticVersionString() + " (Beta) (Staging)";
        }
        else if (buildChannel_ == UPDATE_CHANNEL_GUINEA_PIG)
        {
            return "v" + semanticVersionString() + " (Guinea Pig) (Staging)";
        }
        else
        {
            return "v" + semanticVersionString() + " (Staging)";
        }
#endif
    }
    else
    {
#ifdef USE_BUILD_ID
        return "v" + semanticVersionString() + " (#" + USE_BUILD_ID + ")";
#else
        if (buildChannel_ == UPDATE_CHANNEL_BETA)
        {
            return "v" + semanticVersionString() + " (Beta)";
        }
        else if (buildChannel_ == UPDATE_CHANNEL_GUINEA_PIG)
        {
            return "v" + semanticVersionString() + " (Guinea Pig)";
        }
        else
        {
            return "v" + semanticVersionString();
        }
#endif
    }
}

QString AppVersion::semanticVersionString() const
{
    return major() + "." + minor() + "." + build();
}

AppVersion::AppVersion() :
    version_(QString::asprintf("%d.%d", WS_VERSION_MAJOR, WS_VERSION_MINOR)),
    isStaging_(false),
#ifdef WINDSCRIBE_IS_BETA
    buildChannel_(UPDATE_CHANNEL_BETA)
#elif defined WINDSCRIBE_IS_GUINEA_PIG
    buildChannel_(UPDATE_CHANNEL_GUINEA_PIG)
#else
    buildChannel_(UPDATE_CHANNEL_RELEASE)
#endif
{
}
