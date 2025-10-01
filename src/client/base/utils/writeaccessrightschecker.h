#pragma once

#include <QString>
#include <QTemporaryFile>

class WriteAccessRightsChecker : public QTemporaryFile
{
    Q_OBJECT

public:
    explicit WriteAccessRightsChecker(const QString &dirname);
    bool isWriteable() const;
    bool isElevated() const;

private:
    void testWrite();
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    int realUid();
#endif

    bool is_writeable_;
};
