#ifndef IP4_ADDRESS_AND_MASK_H
#define IP4_ADDRESS_AND_MASK_H

// specifies IPv4 address and mask, so it can be a single ip (if mask = 0xFFFFFFFF) or the range of addresses
class Ip4AddressAndMask
{
public:
    explicit Ip4AddressAndMask(PCWSTR address);	// parse ip4 and mask (example, 123.12.23.34, 1.2.4.5/24, ...)
	explicit Ip4AddressAndMask(PCSTR address);

	bool isValid() const;

	// The IPv4 address is returned in network order (bytes ordered from left to right).
    UINT32 ipNetworkOrder() const;
	UINT32 maskNetworkOrder() const;

	// The IPv4 address is returned in host byte order (bytes ordered from right to left). Use in FWPM. filter functions.
	UINT32 ipHostOrder() const;
	UINT32 maskHostOrder() const;

	std::wstring asString() const;

	bool operator==(const Ip4AddressAndMask& other) const
	{
		return ipAddress_ == other.ipAddress_ && mask_ == other.mask_;
	}

	bool operator <(const Ip4AddressAndMask& other) const
	{
		return (ipAddress_ == other.ipAddress_) ?
			(mask_ < other.mask_) : (ipAddress_ < other.ipAddress_);
	}


private:
	// stored in host order
	UINT32 ipAddress_;		
	UINT32 mask_;

	void constructFromString(const std::string &str);
	UINT32 maskFromInteger(UINT32 num);
};

#endif // IP4_ADDRESS_AND_MASK_H
