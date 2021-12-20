#pragma once

#define SCALE_FACTOR (DPIScale::instance().scaleFactor())

class DPIScale
{
public:
	static DPIScale &instance()
	{
		static DPIScale s;
		return s;
	}


	void recalcScaleFactorWithNewDPIValue(unsigned int newDPI);
	double scaleFactor() const;

private:
	DPIScale();
	double scaleFactor_;
};