#pragma once

class TapAdapterDetector
{
public:

	struct TapAdapterInfo
	{
		DWORD ip;
		NET_LUID luid;
	};


	TapAdapterDetector();
	void start();
	void stop();

	void setCallbackHandler(std::function<void(bool, TapAdapterInfo *)> callback);

private:
	static void __stdcall interfaceChangeCallback(_In_ PVOID CallerContext,
		_In_ PMIB_IPINTERFACE_ROW Row OPTIONAL,
		_In_ MIB_NOTIFICATION_TYPE NotificationType);

	void detectCurrentConfig();
	bool removeAdapter(NET_LUID luid);
	int findAdapter(NET_LUID luid) const;

	void updateState();

	HANDLE notificationHandle_;

	struct AdapterDescr
	{
		NET_LUID luid;
		BOOLEAN isConnected;
		DWORD dwIp;
	};

	std::vector<AdapterDescr> windscribeAdapters_;
	std::function<void(bool, TapAdapterInfo*)> callback_;

};

