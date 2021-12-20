#include "DPIScale.h"
#include <Windows.h>

DPIScale::DPIScale()
{
	HDC hdcMeasure = ::CreateCompatibleDC(NULL);
	scaleFactor_ = (double)::GetDeviceCaps(hdcMeasure, LOGPIXELSX) / (double)USER_DEFAULT_SCREEN_DPI;
	::DeleteDC(hdcMeasure);
}

void DPIScale::recalcScaleFactorWithNewDPIValue(unsigned int newDPI)
{
	scaleFactor_ = (double)newDPI / (double)USER_DEFAULT_SCREEN_DPI;
	int g = 0;
}
double DPIScale::scaleFactor() const
{
	return scaleFactor_;
}
