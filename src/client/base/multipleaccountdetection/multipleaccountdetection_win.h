#pragma once

#include <QString>
#include "secretvalue_win.h"
#include "imultipleaccountdetection.h"

class MultipleAccountDetection_win : public IMultipleAccountDetection
{
public:
    MultipleAccountDetection_win();
    void userBecomeExpired(const QString &username, const QString &userId) override;
    bool entryIsPresent(QString &username, QString &userId) override;
    void removeEntry() override;

private:
    SecretValue_win secretValue_;
};
