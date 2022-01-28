#ifndef SOCKSPROXYREADEXACTLY_H
#define SOCKSPROXYREADEXACTLY_H

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

#endif // SOCKSPROXYREADEXACTLY_H
