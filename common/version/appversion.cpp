#include "appversion.h"
#include "windscribe_version.h"

#include <QStringList>

QString AppVersion::version() const
{
    return version_;
}

QString AppVersion::major() const
{
    return QString::number(WINDSCRIBE_MAJOR_VERSION);
}

QString AppVersion::minor() const
{
    return QString::number(WINDSCRIBE_MINOR_VERSION);
}

QString AppVersion::build() const
{
    return QString::number(WINDSCRIBE_BUILD_VERSION);
}

QString AppVersion::fullVersionString() const
{
    return "v" + semanticVersionString() + " Issue 392 RC1";

    /*
    if (isStaging_)
    {
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
    }
    else
    {
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
    }
    */
}

QString AppVersion::semanticVersionString() const
{
    return major() + "." + minor() + "." + build();
}

AppVersion::AppVersion() :
    version_(QString::asprintf("%d.%d", WINDSCRIBE_MAJOR_VERSION, WINDSCRIBE_MINOR_VERSION)),
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
