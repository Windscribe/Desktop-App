#pragma once

// thread safe wrapper for Windows Filtering Platform handle

class FwpmWrapper
{
public:
	FwpmWrapper();
	~FwpmWrapper();

	bool isInitialized();

	HANDLE getHandleAndLock();
	void unlock();

	bool beginTransaction();
	bool endTransaction();


private:
	HANDLE engineHandle_;
	std::mutex mutex_;
};

