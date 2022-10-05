#include "FontResources.h"
#include "../resource.h"
#include "Application.h"

FontResources::FontResources()
{

}
FontResources::~FontResources()
{
	for (auto it = fontsMap_.begin(); it != fontsMap_.end(); ++it)
	{
		delete it->second;
	}
	for (auto it = gdiFontsMap_.begin(); it != gdiFontsMap_.end(); ++it)
	{
		DeleteObject(it->second);
	}
}

bool FontResources::init()
{
	// loading regular font
	{
		HRSRC res = FindResource(g_application->getInstance(), MAKEINTRESOURCE(IDR_PLEXFONT_REGULAR), L"BINARY");

		HGLOBAL mem = LoadResource(g_application->getInstance(), res);
		void *data = LockResource(mem);
		DWORD len = SizeofResource(g_application->getInstance(), res);

		Gdiplus::Status nResults = fontCollection_.AddMemoryFont(data, len);

		if (nResults != Gdiplus::Ok)
		{
			return false;
		}

		DWORD nFonts;
		if (AddFontMemResourceEx(data, len, NULL,&nFonts) == NULL)
		{
			return false;
		}
	}

	// loading bold font
	{
		HRSRC res = FindResource(g_application->getInstance(), MAKEINTRESOURCE(IDR_PLEXFONT_BOLD), L"BINARY");

		HGLOBAL mem = LoadResource(g_application->getInstance(), res);
		void *data = LockResource(mem);
		DWORD len = SizeofResource(g_application->getInstance(), res);

		Gdiplus::Status nResults = fontCollection_.AddMemoryFont(data, len);

		if (nResults != Gdiplus::Ok)
		{
			return false;
		}

		DWORD nFonts;
		if (AddFontMemResourceEx(data, len, NULL, &nFonts) == NULL)
		{
			return false;
		}
	}
	int nNumFound = 0;
	fontCollection_.GetFamilies(1, &fontFamily_, &nNumFound);

	return true;
}

Gdiplus::Font *FontResources::getFont(int size, bool isBold)
{
	auto it = fontsMap_.find(std::make_pair(size, isBold));
	if (it != fontsMap_.end())
	{
		return it->second;
	}
	else
	{
		Gdiplus::Font *font = new Gdiplus::Font(&fontFamily_, (Gdiplus::REAL)size, isBold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		fontsMap_[std::make_pair(size, isBold)] = font;
		return font;
	}
}

HFONT FontResources::getHFont(int size, bool isBold)
{
	auto it = gdiFontsMap_.find(std::make_pair(size, isBold));
	if (it != gdiFontsMap_.end())
	{
		return it->second;
	}
	else
	{
		HDC screenDC = CreateCompatibleDC(NULL);
		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		lf.lfHeight = -size;
		lf.lfWeight = isBold ? FW_BOLD : FW_NORMAL;
		lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
		wcscpy_s(lf.lfFaceName, L"IBM Plex Sans");

		HFONT hFont = CreateFontIndirect(&lf);
		DeleteDC(screenDC);
		gdiFontsMap_[std::make_pair(size, isBold)] = hFont;
		return hFont;
	}
}