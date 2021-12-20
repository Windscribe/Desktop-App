#include "clientconnectiondescr.h"

ClientConnectionDescr::ClientConnectionDescr() :
    bClientAuthReceived_(false), protocolVersion_(0), clientId_(0), pid_(0), latestCommandTimeMs_(0)
{

}
