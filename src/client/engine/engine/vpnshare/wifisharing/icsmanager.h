#pragma once

#include <QObject>
#include "engine/helper/helper.h"

// IcsManager works through helper, because need admin rights
class IcsManager : public QObject
{
    Q_OBJECT
public:
    explicit IcsManager(QObject *parent, Helper *helper);

    bool isSupportedIcs();
    bool startIcs();
    bool stopIcs();
    bool changeIcs(const QString &adapterName);

private:
    Helper *helper_;
};
