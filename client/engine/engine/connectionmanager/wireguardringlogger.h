/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Adapted from mozilla-vpn-client\src\platforms\windows\daemon\windowstunnellogger.h

#ifndef WIREGUARDRINGLOGGER_H
#define WIREGUARDRINGLOGGER_H

#include <QFile>

namespace wsl
{

class WireguardRingLogger
{
public:
    WireguardRingLogger(const QString& filename);
    ~WireguardRingLogger();

    void getNewLogEntries();

    bool isTunnelRunning() const { return tunnelRunning_; }

private:
    QFile wireguardLogFile_;
    uchar* logData_ = nullptr;
    int ringLogIndex_ = -1;
    quint64 startTime_ = 0; // nanoseconds
    bool tunnelRunning_ = false;

private:
    bool mapWireguardRinglogFile();
    void process(int index);
    int nextIndex();
};

}

#endif  // WIREGUARDRINGLOGGER_H
