#ifndef ICSMANAGER_H
#define ICSMANAGER_H

#include "Engine/Helper/helper_win.h"

// IcsManager works through helper, because need admin rights
class IcsManager : public QObject
{
    Q_OBJECT
public:
    explicit IcsManager(QObject *parent, IHelper *helper);

    bool isSupportedIcs();
    bool startIcs();
    bool stopIcs();
    bool changeIcs(const QString &adapterName);

private:
    Helper_win *helper_;
};

#endif // ICSMANAGER_H
