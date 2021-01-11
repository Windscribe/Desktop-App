#pragma once

class TapAdapterDetector
{
public:
	static bool detect(DWORD &outIp, NET_LUID &outLuid);
};

