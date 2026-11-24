#pragma once

#include <QObject>
#include <QTest>

#include "utils/credentialsvalidation.h"

class TestCredentialsValidation : public QObject
{
    Q_OBJECT

private slots:
    void testCheckUsername_data();
    void testCheckUsername();
    void testCheckPassword_data();
    void testCheckPassword();
};
