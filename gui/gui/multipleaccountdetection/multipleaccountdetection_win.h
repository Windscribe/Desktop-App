#ifndef MULTIPLEACCOUNTDETECTION_WIN_H
#define MULTIPLEACCOUNTDETECTION_WIN_H

#include <QString>
#include "secretvalue_win.h"
#include "imultipleaccountdetection.h"

class MultipleAccountDetection_win : public IMultipleAccountDetection
{
public:
    MultipleAccountDetection_win();
    virtual void userBecomeExpired(const QString &username);
    virtual bool entryIsPresent(QString &username);
    virtual void removeEntry();

private:
    SecretValue_win secretValue_;
};

#endif // MULTIPLEACCOUNTDETECTION_WIN_H
