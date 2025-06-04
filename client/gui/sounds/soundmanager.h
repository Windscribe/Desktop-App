#pragma once

#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QObject>

#include "backend/preferences/preferences.h"

class SoundManager : public QObject
{
    Q_OBJECT
public:
    SoundManager(QObject *parent, Preferences *preferences);

    void play(const CONNECT_STATE &connectState);
    void stop();

private slots:
    void onSoundSettingsChanged(const types::SoundSettings &soundSettings);
    void onPlayerError(QMediaPlayer::Error error, const QString &errorString);
    void onPlayerPositionChanged(qint64 position);

private:
    void playSound(const QUrl &url, QMediaPlayer::Loops loops);

    Preferences *preferences_;
    QMediaPlayer *player_;
    QAudioOutput *audioOutput_;

    bool connectedEventQueued_ = false;
    qint64 position_ = 0;
};
