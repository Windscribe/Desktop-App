#include "all_headers.h"
#include "adapters_info.h"


AdaptersInfo::AdaptersInfo()
{
	ULONG ulAdapterInfoSize = sizeof(IP_ADAPTER_INFO);
	adapterInfoVector_.resize(ulAdapterInfoSize);

	if (GetAdaptersInfo((IP_ADAPTER_INFO *)&adapterInfoVector_[0], &ulAdapterInfoSize) == ERROR_BUFFER_OVERFLOW)
	{
		adapterInfoVector_.resize(ulAdapterInfoSize);
	}

	if (GetAdaptersInfo((IP_ADAPTER_INFO *)&adapterInfoVector_[0], &ulAdapterInfoSize) == ERROR_SUCCESS)
	{
		pAdapterInfo_ = (IP_ADAPTER_INFO *)&adapterInfoVector_[0];
	}
	else
	{
		adapterInfoVector_.clear();
		pAdapterInfo_ = NULL;
	}
}


bool AdaptersInfo::isWindscribeAdapter(NET_IFINDEX index) const
{
	if (pAdapterInfo_)
	{
		IP_ADAPTER_INFO *ai = pAdapterInfo_;
		do
		{
			if (ai->Index == index)
			{
				return strstr(ai->Description, "Windscribe") != 0;
			}
			ai = ai->Next;
		} while (ai);
	}
	return false;
}

bool AdaptersInfo::getWindscribeIkev2AdapterInfo(NET_IFINDEX &outIfIndex, std::string &outIp)
{
	if (pAdapterInfo_)
	{
		IP_ADAPTER_INFO *ai = pAdapterInfo_;
		do
		{
			if (strstr(ai->Description, "Windscribe IKEv2") != 0)
			{
				outIfIndex = ai->Index;
				outIp = ai->IpAddressList.IpAddress.String;
				return true;
			}
			ai = ai->Next;
		} while (ai);
	}
	return false;
}

std::vector<NET_IFINDEX> AdaptersInfo::getTAPAdapters()
{
	std::vector<NET_IFINDEX> list;

	if (pAdapterInfo_)
	{
		IP_ADAPTER_INFO *ai = pAdapterInfo_;

		do
		{
			if (strstr(ai->Description, "Windscribe") != 0)
			{
				list.push_back(ai->Index);
			}
			ai = ai->Next;
		} while (ai);
	}
	return list;
}
