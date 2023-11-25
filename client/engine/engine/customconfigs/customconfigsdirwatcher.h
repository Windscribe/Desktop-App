#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

class CustomConfigsDirWatcher : public QObject
{
    Q_OBJECT
public:
    explicit CustomConfigsDirWatcher(QObject *parent, const QString &path);

    QString curDir() const;
    QStringList curFiles() const;

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

    static constexpr int EMIT_TIMEOUT = 1000;
    QTimer emitTimer_;

    void checkFiles(bool bWithEmitSignal, bool bFileChanged);
};
