#pragma once

#include <QObject>
#include <QTest>

class TestUtilsLinux : public QObject
{
    Q_OBJECT

private slots:
    void testExecCmdPinsLocale();
    void testExecCmdPinsLocaleInEveryPipelineStage();
};
