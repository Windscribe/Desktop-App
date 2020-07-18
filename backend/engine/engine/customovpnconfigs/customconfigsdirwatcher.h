#ifndef CUSTOMCONFIGSDIRWATCHER_H
#define CUSTOMCONFIGSDIRWATCHER_H

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

class CustomConfigsDirWatcher : public QObject
{
    Q_OBJECT
public:
    explicit CustomConfigsDirWatcher(QObject *parent, const QString &path);

    QString curDir() const;

signals:
    void dirChanged();

private slots:
    void onDirectoryChanged(const QString &path);
    void onFileChanged(const QString &path);

    void onEmitTimer();


private:
    QString path_;
    QFileSystemWatcher dirWatcher_;
    QStringList curFiles_;

    const int EMIT_TIMEOUT = 3000;
    QTimer emitTimer_;

    void checkFiles(bool bWithEmitSignal, bool bFileChanged);
};

#endif // CUSTOMCONFIGSDIRWATCHER_H
