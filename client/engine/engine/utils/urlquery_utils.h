#pragma once

#include <QUrlQuery>

namespace urlquery_utils
{
    void addPlatformQueryItems(QUrlQuery &urlQuery);
    void addAuthQueryItems(QUrlQuery &urlQuery, const QString &authHash = QString());
};
