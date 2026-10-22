#pragma once

#include <QObject>
#include <QTest>
#include <QTemporaryDir>

class TestSystemWideAppLock : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void testFreeLock();
    void testAcquire();
    void testHeldByAnotherProcess();
    void testReleasedWhenHolderExits();
#if !defined(Q_OS_WIN)
    void testMissingFile();
    void testReadOnlyFile();
#endif

private:
    std::string name_;
#if !defined(Q_OS_WIN)
    QTemporaryDir dir_;
#endif
};
