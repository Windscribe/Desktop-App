#include "socksproxyreadexactly.h"

namespace SocksProxyServer {

SocksProxyReadExactly::SocksProxyReadExactly(unsigned int length) :
    length_(length)
{

}

bool SocksProxyReadExactly::read(QByteArray &arr)
{
    if ((unsigned int)arr.size() >= (length_ - arr_.size()))
    {
        unsigned int l = length_ - arr_.size();
        arr_.append(arr.data(), l);
        arr.remove(0, l);
        return true;
    }
    else
    {
        arr_.append(arr);
        arr.clear();
        return false;
    }
}

} // namespace SocksProxyServer
