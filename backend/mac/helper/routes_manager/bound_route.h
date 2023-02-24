#ifndef BoundRoute_h
#define BoundRoute_h

#include <string>

class BoundRoute
{
public:
    BoundRoute();
    ~BoundRoute();
    
    void create(const std::string &ipAddress, const std::string &interfaceName);
    void remove();
    
private:
    bool isBoundRouteAdded_;
    std::string ipAddress_;
    std::string interfaceName_;
};

#endif /* BoundRoute_h */
