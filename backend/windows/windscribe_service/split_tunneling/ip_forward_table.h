#pragma once

// wrapper for API GetIpForwardTable and util functions
class IpForwardTable
{
public:
	IpForwardTable();

	DWORD count() const;
	const MIB_IPFORWARDROW *getByIndex(DWORD ind) const;

private:
	std::vector<unsigned char> ipForwardVector_;
	MIB_IPFORWARDTABLE *pIpForwardTable;
};

