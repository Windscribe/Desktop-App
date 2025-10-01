#define MINIAUDIO_IMPLEMENTATION
#include "soundmanager.h"

#include "utils/log/categories.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

SoundManager::SoundManager(QObject *parent, Preferences *preferences)
    : QObject(parent), preferences_(preferences), audioEngineInitialized_(false), soundInitialized_(false), shouldLoop_(false), connectedEventQueued_(false)
{
}

SoundManager::~SoundManager()
{
    cleanupCurrentSoundData();
}

void SoundManager::handleQueuedConnectedEvent()
{
    if (connectedEventQueued_) {
        connectedEventQueued_ = false;
        playSound(preferences_->soundSettings().connectedSoundPath, false);
    }
}

void SoundManager::play(const CONNECT_STATE &connectState)
{
    if (connectState == CONNECT_STATE_CONNECTED) {
        if (preferences_->soundSettings().connectedSoundPath == ":/sounds/Fart_(Deluxe)_on.mp3" && soundInitialized_) {
            qCDebug(LOG_BASIC) << "Connect event, stop loop";
            connectedEventQueued_ = true;
            ma_sound_set_looping(&sound_, MA_FALSE);
            return;
        }
        if (preferences_->soundSettings().connectedSoundType == SOUND_NOTIFICATION_TYPE_NONE) {
            return;
        }
        playSound(preferences_->soundSettings().connectedSoundPath, false);
    } else if (connectState == CONNECT_STATE_DISCONNECTED) {
        if (preferences_->soundSettings().disconnectedSoundType == SOUND_NOTIFICATION_TYPE_NONE) {
            return;
        }
        playSound(preferences_->soundSettings().disconnectedSoundPath, false);
    } else if (connectState == CONNECT_STATE_CONNECTING) {
        if (preferences_->soundSettings().connectedSoundPath == ":/sounds/Fart_(Deluxe)_on.mp3" &&
            preferences_->soundSettings().connectedSoundType == SOUND_NOTIFICATION_TYPE_BUNDLED)
        {
            QString path = preferences_->soundSettings().connectedSoundPath.replace("_on.mp3", "_loop.mp3");
            playSound(path, true);
        }
    } else if (connectState == CONNECT_STATE_DISCONNECTING) {
        stop();
    }
}

void SoundManager::stop()
{
    if (audioEngineInitialized_) {
        ma_sound_stop(&sound_);
    }
}

void SoundManager::playSound(const QString &path, bool loop)
{
    qCDebug(LOG_BASIC) << "Playing sound" << path;

    cleanupCurrentSoundData();

    ma_result result = ma_engine_init(NULL, &audioEngine_);
    if (result != MA_SUCCESS) {
        qCDebug(LOG_BASIC) << "Failed to initialize miniaudio engine: " << result;
        return;
    }
    audioEngineInitialized_ = true;

    if (path.startsWith(":/sounds")) {     // load from resources
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCDebug(LOG_BASIC) << "Failed to load the sound file" << path;
            return;
        }
        audioBuffer_ = file.readAll();
        file.close();

        result = ma_decoder_init_memory(audioBuffer_.data(), audioBuffer_.size(), NULL, &decoder_);
        if (result != MA_SUCCESS) {
            qCDebug(LOG_BASIC) << "ma_decoder_init_memory failed for the sound file" << path << ":" << result;
            return;
        } else {
            decoderInitialized_ = true;
        }

        result = ma_sound_init_from_data_source(&audioEngine_, &decoder_, MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC,
                                       NULL, &sound_);
        if (result != MA_SUCCESS) {
            qCDebug(LOG_BASIC) << "ma_sound_init_from_data_source failed for the sound file" << path << ":" << result;
            return;
        }
    } else {    // load from a file
        result = ma_sound_init_from_file(&audioEngine_, path.toUtf8().constData(),
                                               MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC,
                                               NULL, NULL, &sound_);
        if (result != MA_SUCCESS) {
            qCDebug(LOG_BASIC) << "Failed to load sound file" << path << ":" << result;
            return;
        }
    }

    soundInitialized_ = true;
    shouldLoop_ = loop;
    ma_sound_set_volume(&sound_, 0.7f);
    ma_sound_set_looping(&sound_, loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_end_callback(&sound_, endCallback, this);
    qCDebug(LOG_BASIC) << "Starting playback";
    result = ma_sound_start(&sound_);
    if (result != MA_SUCCESS) {
        qCDebug(LOG_BASIC) << "Failed to start sound playback:" << result;
        ma_sound_uninit(&sound_);
        soundInitialized_ = false;
        return;
    }
}

void SoundManager::cleanupCurrentSoundData()
{
    qCDebug(LOG_BASIC) << "Cleaning up current sound data";
    if (soundInitialized_) {
        ma_sound_stop(&sound_);
        ma_sound_uninit(&sound_);
        soundInitialized_ = false;
    }

    if (decoderInitialized_) {
        ma_decoder_uninit(&decoder_);
        decoderInitialized_ = false;
    }

    if (audioEngineInitialized_) {
        ma_engine_uninit(&audioEngine_);
        audioEngineInitialized_ = false;
    }

    audioBuffer_.clear();
}

void SoundManager::endCallback(void* pUserData, ma_sound* pSound)
{
    SoundManager* manager = static_cast<SoundManager*>(pUserData);
    if (!manager) {
        return;
    }

    // safely release memory from the sound when it has ended
    QTimer::singleShot(0, manager, [manager] () {
        manager->cleanupCurrentSoundData();
    });

    if (manager->connectedEventQueued_) {
        QTimer::singleShot(0, manager, &SoundManager::handleQueuedConnectedEvent);
    }
}
