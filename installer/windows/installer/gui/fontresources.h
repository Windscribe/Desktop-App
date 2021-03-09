#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <map>

class FontResources
{
public:
	FontResources();
	virtual ~FontResources();
	bool init();

	Gdiplus::Font *getFont(int size, bool isBold);
	HFONT getHFont(int size, bool isBold);

private:
	Gdiplus::PrivateFontCollection fontCollection_;
	Gdiplus::FontFamily fontFamily_;

	std::map<std::pair<int, bool>, Gdiplus::Font *> fontsMap_;
	std::map<std::pair<int, bool>, HFONT> gdiFontsMap_;

};
