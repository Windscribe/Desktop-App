#pragma once

#include <QObject>
#include <QTest>

class TestAuthToken : public QObject
{
    Q_OBJECT

private slots:
    void testParse_data();
    void testParse();
};
