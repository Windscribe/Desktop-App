#ifndef PROTON_NETWORK_UTIL_IPADDRESS_H
#define PROTON_NETWORK_UTIL_IPADDRESS_H

class IpAddress
{
private:
    UINT32 pIPv4Address;
public:
    explicit IpAddress(PCWSTR address);
	explicit IpAddress(PCSTR address);
	explicit IpAddress(DWORD dwAddress);

	// The IPv4 address is returned in network order (bytes ordered from left to right).
    UINT32 IPv4NetworkOrder() const;

	// The IPv4 address is returned in host byte order (bytes ordered from right to left). Uses in FWPM.. filter functions.
	UINT32 IPv4HostOrder() const;

	std::wstring asString() const;

	bool operator==(const IpAddress& other) const
	{
		return pIPv4Address == other.pIPv4Address;
	}

	bool operator <(const IpAddress& other) const
	{
		return pIPv4Address < other.pIPv4Address;
	}
};

#endif
