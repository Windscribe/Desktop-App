#pragma once

#include <QObject>
#include <QTest>

class TestUtils : public QObject
{
    Q_OBJECT

private slots:
    void testRedactCredential_data();
    void testRedactCredential();
};
