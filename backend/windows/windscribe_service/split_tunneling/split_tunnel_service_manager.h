#pragma once

class SplitTunnelServiceManager
{
public:
	SplitTunnelServiceManager();

	void start();
	void stop();
	bool isStarted() const { return bStarted_; }


private:
	bool bStarted_;
};

