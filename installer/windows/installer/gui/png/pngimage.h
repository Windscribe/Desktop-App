#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <gdiplus.h>

extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;

class PngImage
{
public:
	PngImage(const std::wstring &resourceName, int width, int height);
	~PngImage();

	Gdiplus::Bitmap *getBitmap() { return m_pBitmap; }

private:

	IStream *m_pStream;
	Gdiplus::Bitmap* m_pBitmap;

	bool GetResource(LPCTSTR lpName, LPCTSTR lpType, void* pResource, int& nBufSize);
	bool LoadFromBuffer(BYTE* pBuff, int nSize);
	
};

