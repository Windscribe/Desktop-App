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

    workerThread_ = new QThread(this);
    worker_ = new SoundPlayerWorker();
    worker_->moveToThread(workerThread_);

    connect(this, &SoundManager::preparePlayer, worker_, &SoundPlayerWorker::preparePlayer);
    connect(worker_, &SoundPlayerWorker::playerReady, this, &SoundManager::onPlayerReady);

    workerThread_->start();
}

SoundManager::~SoundManager()
{
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait();
        delete worker_;
    }
}

void SoundManager::onSoundSettingsChanged(const types::SoundSettings &soundSettings)
{
}

void SoundManager::onPlayerReady(QMediaPlayer *player)
{
    if (player_) {
        player_->stop();
        player_->deleteLater();
    }
    player_ = player;

    connect(player_, &QMediaPlayer::errorOccurred, this, &SoundManager::onPlayerError);
    connect(player_, &QMediaPlayer::positionChanged, this, &SoundManager::onPlayerPositionChanged);
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
        if (preferences_->soundSettings().connectedSoundType == SOUND_NOTIFICATION_TYPE_NONE) {
            return;
        } else if (preferences_->soundSettings().connectedSoundType == SOUND_NOTIFICATION_TYPE_BUNDLED) {
            url = QUrl("qrc" + preferences_->soundSettings().connectedSoundPath);
        } else {
            url = QUrl(preferences_->soundSettings().connectedSoundPath);
        }
        emit preparePlayer(url, QMediaPlayer::Once);
    } else if (connectState == CONNECT_STATE_DISCONNECTED) {
        QUrl url;
        if (preferences_->soundSettings().connectedSoundType == SOUND_NOTIFICATION_TYPE_NONE) {
            return;
        } else if (preferences_->soundSettings().disconnectedSoundType == SOUND_NOTIFICATION_TYPE_BUNDLED) {
            url = QUrl("qrc" + preferences_->soundSettings().disconnectedSoundPath);
        } else {
            url = QUrl(preferences_->soundSettings().disconnectedSoundPath);
        }
        emit preparePlayer(url, QMediaPlayer::Once);
    } else if (connectState == CONNECT_STATE_CONNECTING) {
        if (preferences_->soundSettings().connectedSoundPath == ":/sounds/Fart_(Deluxe)_on.mp3" &&
            preferences_->soundSettings().connectedSoundType == SOUND_NOTIFICATION_TYPE_BUNDLED)
        {
            QUrl url = QUrl("qrc" + preferences_->soundSettings().connectedSoundPath.replace("_on.mp3", "_loop.mp3"));
            emit preparePlayer(url, QMediaPlayer::Infinite);
        }
    } else if (connectState == CONNECT_STATE_DISCONNECTING) {
        stop();
    }
}

void SoundManager::stop()
{
    if (player_) {
        player_->stop();
        player_->deleteLater();
        player_ = nullptr;
    }
}

void SoundManager::onPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    qCDebug(LOG_BASIC) << "QMediaPlayer error: " << error << " " << errorString;
    stop();
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
        emit preparePlayer(url, QMediaPlayer::Once);
    }
    position_ = position;
}

// SoundPlayerWorker implementation
SoundPlayerWorker::SoundPlayerWorker(QObject *parent)
    : QObject(parent)
{
}

void SoundPlayerWorker::preparePlayer(const QUrl &url, QMediaPlayer::Loops loops)
{
    QMediaPlayer *player = new QMediaPlayer();
    QAudioOutput *audioOutput = new QAudioOutput();
    audioOutput->setVolume(0.7);
    player->setAudioOutput(audioOutput);
    audioOutput->setDevice(QAudioDevice());
    player->setSource(url);
    player->setLoops(loops);

    emit playerReady(player);
}
