#ifndef WRITEACCESSRIGHTSCHECKER_H
#define WRITEACCESSRIGHTSCHECKER_H

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
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    int realUid();
#endif

    bool is_writeable_;
};

#endif // WRITEACCESSRIGHTSCHECKER_H
