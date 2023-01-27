/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Adapted from mozilla-vpn-client\src\platforms\windows\daemon\windowstunnellogger.cpp

#include <QDateTime>
#include <QScopeGuard>

#include "wireguardringlogger.h"
#include "utils/logger.h"


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

constexpr uint32_t WG_LOG_INDEX_OFFSET = 4;
constexpr uint32_t WG_LOG_HEADER_SIZE = 8;
constexpr uint32_t WG_LOG_MESSAGE_RING_SIZE = 2048;
constexpr uint32_t WG_LOG_MESSAGE_SIZE = 512;
constexpr uint32_t WG_LOG_TIMESTAMP_SIZE = 8;
constexpr uint32_t WG_LOG_FILE_SIZE =
    WG_LOG_HEADER_SIZE + ((WG_LOG_MESSAGE_SIZE + WG_LOG_TIMESTAMP_SIZE) * WG_LOG_MESSAGE_RING_SIZE);


WireguardRingLogger::WireguardRingLogger(const QString& filename)
    : wireguardLogFile_(filename)
{
    startTime_ = QDateTime::currentMSecsSinceEpoch() * 1000000;
    ringLogIndex_ = -1;
}

WireguardRingLogger::~WireguardRingLogger()
{
    if (logData_ != nullptr)
    {
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

    if (!wireguardLogFile_.open(QIODeviceBase::ReadOnly))
    {
        qCDebug(LOG_CONNECTION) << "Failed to open the wireguard service log file";
        return false;
    }

    auto guard = qScopeGuard([&]
    {
        if (logData_ != nullptr)
        {
            wireguardLogFile_.unmap(logData_);
            logData_ = nullptr;
        }

        wireguardLogFile_.close();
    });

    if (wireguardLogFile_.size() != WG_LOG_FILE_SIZE)
    {
        qCDebug(LOG_CONNECTION) << "The wireguard service log file size is incorrect: expected" <<
                                   WG_LOG_FILE_SIZE << "bytes, actual size" << wireguardLogFile_.size() << "bytes";
        return false;
    }

    logData_ = wireguardLogFile_.map(0, WG_LOG_FILE_SIZE);
    if (!logData_)
    {
        qCDebug(LOG_CONNECTION) << "Failed to map the wireguard service log file";
        return false;
    }

    qCDebug(LOG_CONNECTION) << "Opened and mapped the wireguard service log file" << wireguardLogFile_.fileName();

    const uint32_t magicHeader = 0xbadbabe;
    if (memcmp(logData_, &magicHeader, sizeof(uint32_t)) != 0)
    {
        qCDebug(LOG_CONNECTION) << "Incorrect magic header value in wireguard service log file" << QString::number(*(uint32_t*)logData_, 16);
        return false;
    }

    guard.dismiss();

    return true;
}

int WireguardRingLogger::nextIndex()
{
    qint32 value;
    memcpy(&value, logData_ + WG_LOG_INDEX_OFFSET, 4);
    return value % WG_LOG_MESSAGE_RING_SIZE;
}

void WireguardRingLogger::process(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < WG_LOG_MESSAGE_RING_SIZE);
    size_t offset = index * (WG_LOG_TIMESTAMP_SIZE + WG_LOG_MESSAGE_SIZE);
    uchar* data = logData_ + WG_LOG_HEADER_SIZE + offset;

    quint64 timestamp; // nanoseconds
    memcpy(&timestamp, data, 8);
    if (timestamp <= startTime_) {
        return;
    }

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp / 1000000, Qt::UTC);
    QByteArray message((const char*)data + WG_LOG_TIMESTAMP_SIZE, WG_LOG_MESSAGE_SIZE);
    qCDebug(LOG_WIREGUARD()) << dt.toString("ddMMyy hh:mm:ss:zzz") << QString::fromUtf8(message);

    if (!tunnelRunning_ && message.contains("Keypair 1 created for peer 1")) {
        tunnelRunning_ = true;
    }
}

void WireguardRingLogger::getNewLogEntries()
{
    if (!mapWireguardRinglogFile()) {
        return;
    }

    /* On the first pass, scan all log messages. */
    if (ringLogIndex_ < 0)
    {
        ringLogIndex_ = nextIndex();
        process(ringLogIndex_);
        ringLogIndex_ = (ringLogIndex_ + 1) % WG_LOG_MESSAGE_RING_SIZE;
    }

    /* Report new messages. */
    while (ringLogIndex_ != nextIndex())
    {
        process(ringLogIndex_);
        ringLogIndex_ = (ringLogIndex_ + 1) % WG_LOG_MESSAGE_RING_SIZE;
    }
}

}
