#ifndef MULTIPLEACCOUNTDETECTION_WIN_H
#define MULTIPLEACCOUNTDETECTION_WIN_H

#include <QString>
#include "secretvalue_win.h"
#include "imultipleaccountdetection.h"

class MultipleAccountDetection_win : public IMultipleAccountDetection
{
public:
    MultipleAccountDetection_win();
    void userBecomeExpired(const QString &username) override;
    bool entryIsPresent(QString &username) override;
    void removeEntry() override;

private:
    SecretValue_win secretValue_;
};

#endif // MULTIPLEACCOUNTDETECTION_WIN_H
