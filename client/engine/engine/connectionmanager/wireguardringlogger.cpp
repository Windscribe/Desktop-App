/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Adapted from mozilla-vpn-client\src\platforms\windows\daemon\windowstunnellogger.cpp

#include "wireguardringlogger.h"

#include <QDateTime>
#include <QScopeGuard>

#include "utils/logger.h"
#include "utils/ws_assert.h"


/* The ring logger format used by the Wireguard DLL is as follows, assuming no padding:
 *
 * struct {
 *   uint32_t magic;
 *   uint32_t index;
 *   struct {
 *     uint64_t timestamp;
 *     char message[512];
 *   } ring[2048];
 * };
 */

namespace wsl
{

constexpr uint32_t kWGLogIndexOffset = 4;
constexpr uint32_t kWGLogHeaderSize = 8;
constexpr uint32_t kWGLogMessageRingSize = 2048;
constexpr uint32_t kWGLogMessageSize = 512;
constexpr uint32_t kWGLogTimestampSize = 8;
constexpr uint32_t kWGLogFileSize =
    kWGLogHeaderSize + ((kWGLogMessageSize + kWGLogTimestampSize) * kWGLogMessageRingSize);


WireguardRingLogger::WireguardRingLogger(const QString& filename)
    : wireguardLogFile_(filename)
{
    startTime_ = QDateTime::currentMSecsSinceEpoch() * 1000000;
}

WireguardRingLogger::~WireguardRingLogger()
{
    if (logData_ != nullptr) {
        wireguardLogFile_.unmap(logData_);
        logData_ = nullptr;
    }

    wireguardLogFile_.close();
}

bool
WireguardRingLogger::mapWireguardRinglogFile()
{
    if (logData_) {
        return true;
    }

    // The wireguard service may not have created the log file yet.
    if (!wireguardLogFile_.exists()) {
        return false;
    }

    if (!wireguardLogFile_.open(QIODevice::ReadOnly)) {
        qCDebug(LOG_CONNECTION) << "Failed to open the wireguard service log file";
        return false;
    }

    auto guard = qScopeGuard([&] {
        if (logData_ != nullptr) {
            wireguardLogFile_.unmap(logData_);
            logData_ = nullptr;
        }

        wireguardLogFile_.close();
    });

    if (wireguardLogFile_.size() != kWGLogFileSize) {
        qCDebug(LOG_CONNECTION) << "The wireguard service log file size is incorrect: expected" <<
                                   kWGLogFileSize << "bytes, actual size" << wireguardLogFile_.size() << "bytes";
        return false;
    }

    logData_ = wireguardLogFile_.map(0, kWGLogFileSize);
    if (!logData_) {
        qCDebug(LOG_CONNECTION) << "Failed to map the wireguard service log file";
        return false;
    }

    qCDebug(LOG_CONNECTION) << "Opened and mapped the wireguard service log file" << wireguardLogFile_.fileName();

    const uint32_t magicHeader = 0xbadbabe;
    if (memcmp(logData_, &magicHeader, sizeof(uint32_t)) != 0) {
        qCDebug(LOG_CONNECTION) << "Incorrect magic header value in wireguard service log file" << QString::number(*(uint32_t*)logData_, 16);
        return false;
    }

    guard.dismiss();

    return true;
}

int WireguardRingLogger::nextIndex()
{
    qint32 value;
    memcpy(&value, logData_ + kWGLogIndexOffset, sizeof(value));
    return value % kWGLogMessageRingSize;
}

void WireguardRingLogger::process(int index)
{
    WS_ASSERT(index >= 0);
    WS_ASSERT(index < kWGLogMessageRingSize);
    size_t offset = static_cast<size_t>(index) * (kWGLogTimestampSize + kWGLogMessageSize);
    uchar* data = logData_ + kWGLogHeaderSize + offset;

    quint64 timestamp; // nanoseconds
    memcpy(&timestamp, data, sizeof(timestamp));
    if (timestamp <= startTime_) {
        return;
    }

    const char* msgData = (const char*)data + kWGLogTimestampSize;
    size_t msgLen = qstrnlen(msgData, kWGLogMessageSize);

    if (msgLen > 0) {
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp / 1000000, Qt::UTC);
        QByteArray message(msgData, msgLen);
        qCDebug(LOG_WIREGUARD()) << dt.toString("ddMMyy hh:mm:ss:zzz") << message;

        if (!tunnelRunning_ && message.contains("Keypair 1 created for peer 1")) {
            tunnelRunning_ = true;
        }

        if (message.contains("Failed to setup adapter")) {
            adapterSetupFailed_ = true;
        }
        else if (message.contains("Handshake for peer") && message.contains("did not complete after")) {
            handshakeFailed_ = true;
        }
    }
}

void WireguardRingLogger::getNewLogEntries()
{
    if (!mapWireguardRinglogFile()) {
        return;
    }

    handshakeFailed_ = false;

    /* On the first pass, scan all log messages. */
    if (ringLogIndex_ < 0) {
        ringLogIndex_ = nextIndex();
        process(ringLogIndex_);
        ringLogIndex_ = (ringLogIndex_ + 1) % kWGLogMessageRingSize;
    }

    /* Report new messages. */
    while (ringLogIndex_ != nextIndex()) {
        process(ringLogIndex_);
        ringLogIndex_ = (ringLogIndex_ + 1) % kWGLogMessageRingSize;
    }
}

}
