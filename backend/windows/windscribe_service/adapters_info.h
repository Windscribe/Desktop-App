#pragma once

// wrapper for API GetAdaptersInfo and util functions
class AdaptersInfo
{
public:
	AdaptersInfo();

	bool isWindscribeAdapter(NET_IFINDEX index) const;
	bool getWindscribeIkev2AdapterInfo(NET_IFINDEX &outIfIndex, std::string &outIp);
	std::vector<NET_IFINDEX> getTAPAdapters();

private:
	std::vector<unsigned char> adapterInfoVector_;
	IP_ADAPTER_INFO *pAdapterInfo_;
};

