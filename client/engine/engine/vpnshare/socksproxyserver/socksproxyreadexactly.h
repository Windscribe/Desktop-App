#pragma once

#include <QByteArray>

namespace SocksProxyServer {

class SocksProxyReadExactly
{
public:
    explicit SocksProxyReadExactly(unsigned int length);

    bool read(QByteArray &arr);
    QByteArray &getArr() { return arr_; }

private:
    unsigned int length_;
    QByteArray arr_;
};

} // namespace SocksProxyServer
