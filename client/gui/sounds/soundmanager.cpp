#include "soundmanager.h"

#include <QAudioDevice>

SoundManager::SoundManager(QObject *parent, Preferences *preferences)
    : QObject(parent), preferences_(preferences), player_(nullptr)
{
#ifdef Q_OS_MACOS
    // the default of "ffmpeg" does not seem to play anything, use native backend
    setenv("QT_MEDIA_BACKEND", "darwin", 1);
#endif
    connect(preferences_, &Preferences::soundSettingsChanged, this, &SoundManager::onSoundSettingsChanged);
}

void SoundManager::onSoundSettingsChanged(const types::SoundSettings &soundSettings)
{
}

void SoundManager::playSound(const QUrl &url, QMediaPlayer::Loops loops)
{
    // Unfortunately, setting the device (even a default-constructed QAudioDevice) directly on an existing player does not seem to work;
    // In that case, audioOutput_->device().description() shows the correct device, but play() does not work.
    // So we do the ugly thing here and create a new player for every sound.
    if (player_ != nullptr) {
        disconnect(player_);
        player_->stop();
        player_->deleteLater();
        player_ = nullptr;
    }
    player_ = new QMediaPlayer(this);
    connect(player_, &QMediaPlayer::errorOccurred, this, &SoundManager::onPlayerError);
    connect(player_, &QMediaPlayer::positionChanged, this, &SoundManager::onPlayerPositionChanged);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    audioOutput->setVolume(0.7);
    player_->setAudioOutput(audioOutput);
    audioOutput->setDevice(QAudioDevice());
    player_->setSource(url);
    player_->setLoops(loops);
    player_->play();
}

void SoundManager::play(const CONNECT_STATE &connectState)
{
    if (connectState == CONNECT_STATE_CONNECTED) {
        if (preferences_->soundSettings().connectedSoundPath == ":/sounds/Fart_(Deluxe)_on.mp3" && player_ && player_->isPlaying()) {
            connectedEventQueued_ = true;
            return;
        }
        QUrl url;
        if (preferences_->soundSettings().connectedSoundType == SOUND_NOTIFICATION_TYPE_BUNDLED) {
            url = QUrl("qrc" + preferences_->soundSettings().connectedSoundPath);
        } else {
            url = QUrl(preferences_->soundSettings().connectedSoundPath);
        }
        playSound(url, QMediaPlayer::Once);
    } else if (connectState == CONNECT_STATE_DISCONNECTED) {
        QUrl url;
        if (preferences_->soundSettings().disconnectedSoundType == SOUND_NOTIFICATION_TYPE_BUNDLED) {
            url = QUrl("qrc" + preferences_->soundSettings().disconnectedSoundPath);
        } else {
            url = QUrl(preferences_->soundSettings().disconnectedSoundPath);
        }
        playSound(url, QMediaPlayer::Once);
    } else if (connectState == CONNECT_STATE_CONNECTING) {
        if (preferences_->soundSettings().connectedSoundPath == ":/sounds/Fart_(Deluxe)_on.mp3") {
            QUrl url = QUrl("qrc" + preferences_->soundSettings().connectedSoundPath.replace("_on.mp3", "_loop.mp3"));
            playSound(url, QMediaPlayer::Infinite);
        }
    } else if (connectState == CONNECT_STATE_DISCONNECTING) {
        if (player_) {
            player_->stop();
        }
    }
}

void SoundManager::stop()
{
    if (player_) {
        player_->stop();
    }
}

void SoundManager::onPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    qCDebug(LOG_BASIC) << "QMediaPlayer error: " << error << " " << errorString;
}

void SoundManager::onPlayerPositionChanged(qint64 position)
{
    if (!player_ || player_->loops() != QMediaPlayer::Infinite || !connectedEventQueued_) {
        return;
    }

    if (position < position_) {
        // Indicates that we're looping.  Check if we've queued a connected event.
        connectedEventQueued_ = false;
        QUrl url = QUrl("qrc" + preferences_->soundSettings().connectedSoundPath);
        player_->setSource(url);
        player_->setLoops(QMediaPlayer::Once);
        player_->play();
    }

    position_ = position;
}
