#ifndef RESETWINDSCRIBETAP_WIN_H
#define RESETWINDSCRIBETAP_WIN_H

#include <QString>

class IHelper;

class ResetWindscribeTap_win
{
public:
    static void resetAdapter(IHelper *helper, const QString &tapName);
};

#endif // RESETWINDSCRIBETAP_WIN_H
