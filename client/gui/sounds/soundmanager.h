#pragma once

#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QObject>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

#include "backend/preferences/preferences.h"

class SoundPlayerWorker;

class SoundManager : public QObject
{
    Q_OBJECT
public:
    SoundManager(QObject *parent, Preferences *preferences);
    ~SoundManager();

    void play(const CONNECT_STATE &connectState);
    void stop();

private slots:
    void onSoundSettingsChanged(const types::SoundSettings &soundSettings);
    void onPlayerError(QMediaPlayer::Error error, const QString &errorString);
    void onPlayerPositionChanged(qint64 position);
    void onPlayerReady(QMediaPlayer *player);

signals:
    void preparePlayer(const QUrl &url, QMediaPlayer::Loops loops);

private:
    Preferences *preferences_;
    QMediaPlayer *player_;

    QThread *workerThread_;
    SoundPlayerWorker *worker_;

    bool connectedEventQueued_ = false;
    qint64 position_ = 0;
};

class SoundPlayerWorker : public QObject
{
    Q_OBJECT
public:
    SoundPlayerWorker(QObject *parent = nullptr);

public slots:
    void preparePlayer(const QUrl &url, QMediaPlayer::Loops loops);

signals:
    void playerReady(QMediaPlayer *player);
};
