#pragma once

#include <QObject>
#include <QTimer>

#include "backend/preferences/preferences.h"

#include <miniaudio.h>

class SoundManager : public QObject
{
    Q_OBJECT
public:
    SoundManager(QObject *parent, Preferences *preferences);
    ~SoundManager();

    void play(const CONNECT_STATE &connectState);
    void stop();

private slots:
    void handleQueuedConnectedEvent();

private:
    static void endCallback(void* pUserData, ma_sound* pSound);
    void playSound(const QString &path, bool loop);
    void cleanupCurrentSoundData();

    Preferences *preferences_;
    ma_engine audioEngine_;
    ma_sound sound_;
    ma_decoder decoder_;
    QByteArray audioBuffer_;
    bool audioEngineInitialized_ = false;
    bool soundInitialized_ = false;
    bool decoderInitialized_ = false;
    bool shouldLoop_ = false;
    bool connectedEventQueued_ = false;
};
