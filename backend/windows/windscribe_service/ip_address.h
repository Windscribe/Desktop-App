#ifndef PROTON_NETWORK_UTIL_IPADDRESS_H
#define PROTON_NETWORK_UTIL_IPADDRESS_H

class IpAddress
{
private:
    UINT32 pIPv4Address;
    UINT16 port;
public:
    explicit IpAddress(PCWSTR address);
	explicit IpAddress(PCSTR address);
	explicit IpAddress(DWORD dwAddress, UINT16 wPort);

	// The IPv4 address is returned in network order (bytes ordered from left to right).
    UINT32 IPv4NetworkOrder() const;

	// The IPv4 address is returned in host byte order (bytes ordered from right to left). Uses in FWPM.. filter functions.
	UINT32 IPv4HostOrder() const;

    // The port is returned in network order (bytes ordered from left to right).
    UINT16 portNetworkOrder() const;

    // The port is returned in host byte order (bytes ordered from right to left).
    UINT16 portHostOrder() const;

	std::wstring asString() const;

	bool operator==(const IpAddress& other) const
	{
		return pIPv4Address == other.pIPv4Address && port == other.port;
	}

	bool operator <(const IpAddress& other) const
	{
		return (pIPv4Address == other.pIPv4Address) ?
            (port < other.port) : (pIPv4Address < other.pIPv4Address);
	}
};

#endif
