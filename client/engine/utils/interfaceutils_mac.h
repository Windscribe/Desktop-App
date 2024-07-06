#include <QString>
#include <QVector>

#include "engine/helper/helper_mac.h"
#include "types/networkinterface.h"

class InterfaceUtils_mac
{
public:
    static InterfaceUtils_mac &instance()
    {
        static InterfaceUtils_mac iu;
        return iu;
    }

    void setHelper(Helper_mac *helper);

    const types::NetworkInterface currentNetworkInterface();
    QVector<types::NetworkInterface> currentNetworkInterfaces(bool includeNoInterface);
    QVector<types::NetworkInterface> currentSpoofedInterfaces();

private:
    InterfaceUtils_mac();
    ~InterfaceUtils_mac();

    Helper_mac *helper_;
};