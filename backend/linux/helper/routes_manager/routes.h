#ifndef Routes_h
#define Routes_h

#include <string>
#include <vector>


// helper for add and clear routes via system "route" command
class Routes
{
public:
    void add(const std::string &ip, const std::string &gateway, const std::string &mask);
    void addWithInterface(const std::string &ip, const std::string &interface, const std::string &mask);
    void clear();

private:
    struct RouteDescr
    {
        std::string ip;
        std::string gateway;
        std::string mask;
        std::string interface;
    };

    std::vector<RouteDescr> routes_;
};

#endif /* Routes_h */
