#pragma once

#include <QObject>
#include <QTest>

class TestCliCommands : public QObject
{
    Q_OBJECT

private slots:
    // An unreadable body must not read as "show" or as an import of nothing.
    void testSetHydraProfileRejectsUnreadableBody();
    void testSetHydraProfileRoundTrip();
};
