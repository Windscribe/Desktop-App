#ifndef Routes_h
#define Routes_h

#include "ip_forward_table.h"

// functions for add/delete routes and revert back all operations
class Routes
{
public:
    Routes();
    
	void deleteRoute(const IpForwardTable &curRouteTable, const std::string &destIp, const std::string &maskIp, const std::string &gatewayIp, unsigned long ifIndex);
	void addRoute(const IpForwardTable &curRouteTable, const std::string &destIp, const std::string &maskIp, const std::string &gatewayIp, unsigned long ifIndex, bool useMaxMetric);

	void revertRoutes();

private:
	std::vector<MIB_IPFORWARDROW> deletedRoutes_;
	std::vector<MIB_IPFORWARDROW> addedRoutes_;
    
};

#endif /* Routes_h */
