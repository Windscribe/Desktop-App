#pragma once

#include <string>
#include <vector>


// helper for add and clear routes via system "route" command
class Routes
{
public:
    void add(const std::string &ip, const std::string &gateway, const std::string &mask);
    void addWithInterface(const std::string &ip, const std::string &interface);
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
