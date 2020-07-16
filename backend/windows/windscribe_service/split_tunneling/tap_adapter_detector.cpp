#include "../all_headers.h"

#include "tap_adapter_detector.h"
#include "../adapters_info.h"
#include "ip_address_table.h"

void __stdcall TapAdapterDetector::interfaceChangeCallback(
	_In_ PVOID CallerContext,
	_In_ PMIB_IPINTERFACE_ROW Row OPTIONAL,
	_In_ MIB_NOTIFICATION_TYPE NotificationType
)
{
	TapAdapterDetector *this_ = (TapAdapterDetector *)CallerContext;
	if (Row != NULL)
	{
		if (NotificationType == MibParameterNotification)
		{
			//printf("interfaceChangeCallback MibParameterNotification: %d\n", Row->InterfaceIndex);

			int ind = this_->findAdapter(Row->InterfaceLuid);
			if (ind != -1)
			{
				MIB_IPINTERFACE_ROW localRow;
				localRow.Family = Row->Family;
				localRow.InterfaceLuid = Row->InterfaceLuid;
				localRow.InterfaceIndex = Row->InterfaceIndex;
				if (GetIpInterfaceEntry(&localRow) != NO_ERROR)
				{
					return;
				}
				else
				{
					AdaptersInfo adaptersInfo;
					assert(adaptersInfo.isWindscribeAdapter(Row->InterfaceIndex));
				}
				if (this_->windscribeAdapters_[ind].isConnected != localRow.Connected)
				{
					this_->windscribeAdapters_[ind].isConnected = localRow.Connected;
					if (this_->windscribeAdapters_[ind].isConnected)
					{
						IpAddressTable addrTable;
						this_->windscribeAdapters_[ind].dwIp = addrTable.getAdapterIpAddress(localRow.InterfaceIndex);

						if (this_->windscribeAdapters_[ind].dwIp == 0)
						{
							this_->windscribeAdapters_[ind].isConnected = false;
						}
					}
					this_->updateState();
				}
			}
		}
		else if (NotificationType == MibAddInstance)
		{
			//printf("interfaceChangeCallback MibAddInstance\n");
			AdaptersInfo adaptersInfo;
			if (adaptersInfo.isWindscribeAdapter(Row->InterfaceIndex))
			{
				assert(this_->findAdapter(Row->InterfaceLuid) == -1);

				MIB_IPINTERFACE_ROW localRow;
				localRow.Family = Row->Family;
				localRow.InterfaceLuid = Row->InterfaceLuid;
				localRow.InterfaceIndex = Row->InterfaceIndex;
				if (GetIpInterfaceEntry(&localRow) != NO_ERROR)
				{
					return;
				}

				AdapterDescr ad;
				ad.luid = localRow.InterfaceLuid;
				ad.isConnected = localRow.Connected;
				if (ad.isConnected)
				{
					IpAddressTable addrTable;
					ad.dwIp = addrTable.getAdapterIpAddress(Row->InterfaceIndex);
					if (ad.dwIp == 0)
					{
						ad.isConnected = false;
					}
				}
				
				this_->windscribeAdapters_.push_back(ad);
				this_->updateState();
			}
			
		}
		else if (NotificationType == MibDeleteInstance)
		{
			//printf("interfaceChangeCallback MibDeleteInstance\n");
			if (this_->removeAdapter(Row->InterfaceLuid))
			{
				this_->updateState();
			}
		}
	}
}

TapAdapterDetector::TapAdapterDetector() : notificationHandle_(NULL)
{

}

void TapAdapterDetector::start()
{
	detectCurrentConfig();
	updateState();
	if (NotifyIpInterfaceChange(AF_INET, (PIPINTERFACE_CHANGE_CALLBACK)interfaceChangeCallback, this, FALSE, &notificationHandle_) != NO_ERROR)
	{
		notificationHandle_ = NULL;
	}
}

void TapAdapterDetector::stop()
{
	if (notificationHandle_)
	{
		CancelMibChangeNotify2(notificationHandle_);
		notificationHandle_ = NULL;
	}
}

void TapAdapterDetector::setCallbackHandler(std::function<void(bool, TapAdapterInfo *)> callback)
{
	callback_ = callback;
}

bool TapAdapterDetector::removeAdapter(NET_LUID luid)
{
	for (auto it = windscribeAdapters_.begin(); it != windscribeAdapters_.end(); ++it)
	{
		if (it->luid.Value == luid.Value)
		{
			windscribeAdapters_.erase(it);
			return true;
		}
	}
	return false;
}

int TapAdapterDetector::findAdapter(NET_LUID luid)
{
	for (size_t i = 0; i < windscribeAdapters_.size(); ++i)
	{
		if (windscribeAdapters_[i].luid.Value == luid.Value)
		{
			return i;
		}
	}
	return -1;
}

void TapAdapterDetector::updateState()
{
	for (auto it = windscribeAdapters_.begin(); it != windscribeAdapters_.end(); ++it)
	{
		if (it->isConnected)
		{
			TapAdapterInfo *t = new TapAdapterInfo();
			t->ip = it->dwIp;
			t->luid = it->luid;
			callback_(true, t);

			//IN_ADDR IPAddr;
			//IPAddr.S_un.S_addr = (u_long)it->dwIp;
			//printf("Connected, IP = %s, IP2 = %s\n", it->ip.c_str(), inet_ntoa(IPAddr));

			return;
		}
	}
	
	callback_(false, NULL);
}

void TapAdapterDetector::detectCurrentConfig()
{
	windscribeAdapters_.clear();
	PMIB_IPINTERFACE_TABLE pipTable = NULL;
	DWORD dwRetVal = GetIpInterfaceTable(AF_INET, &pipTable);
	if (dwRetVal != NO_ERROR) 
	{
		return;
	}

	AdaptersInfo adaptersInfo;
	IpAddressTable addrTable;

	for (ULONG i = 0; i < pipTable->NumEntries; i++)
	{
		if (adaptersInfo.isWindscribeAdapter(pipTable->Table[i].InterfaceIndex))
		{
			int g = sizeof(NET_LUID);
			AdapterDescr ad;
			ad.luid = pipTable->Table[i].InterfaceLuid;
			ad.isConnected = pipTable->Table[i].Connected;
			if (ad.isConnected)
			{
				ad.dwIp = addrTable.getAdapterIpAddress(pipTable->Table[i].InterfaceIndex);
			}
			windscribeAdapters_.push_back(ad);
		}
	}
}