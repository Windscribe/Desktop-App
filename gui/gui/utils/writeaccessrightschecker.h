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

private:
    bool isElevated() const;
    void testWrite();

    bool is_writeable_;
};

#endif // WRITEACCESSRIGHTSCHECKER_H
