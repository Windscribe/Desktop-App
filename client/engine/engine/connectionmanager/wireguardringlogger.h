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
    quint64 startTime_ = 0;
    bool tunnelRunning_ = false;

private:
    bool mapWireguardRinglogFile();
    void process(int index);
    int nextIndex();
};

}

#endif  // WIREGUARDRINGLOGGER_H
