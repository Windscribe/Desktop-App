#include "PngImage.h"

#include "../dpiscale.h"
#include <iostream>

const int WINDOW_WIDTH = (int)(350 * SCALE_FACTOR);
const int WINDOW_HEIGHT = (int)(350 * SCALE_FACTOR);

PngImage::PngImage(const std::wstring &resourceName, int width, int height)
{

	// Load gif from resource
	std::wstring sResourceType = L"PNG";
	bool bResult = false;
	BYTE*	pBuff = NULL;
	int		nSize = 0;
	IStream * stream = NULL;

	if (GetResource(resourceName.c_str(), sResourceType.c_str(), pBuff, nSize))
	{
		if (nSize > 0)
		{
			pBuff = new BYTE[nSize];

			if (GetResource(resourceName.c_str(), sResourceType.c_str(), pBuff, nSize))
			{
				if (LoadFromBuffer(pBuff, nSize))
				{
					bResult = true;
				}
			}

			delete[] pBuff;
		}
	}

	if (bResult)
	{
		Gdiplus::Image *image = new Gdiplus::Image(m_pStream, false);
		m_pBitmap = new Gdiplus::Bitmap(width, height);

		// draw
		Gdiplus::Graphics *g = Gdiplus::Graphics::FromImage(m_pBitmap);
		g->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
		g->DrawImage(image, 0, 0, width, height);
		
		delete g;
		delete image;
	}
}

PngImage::~PngImage()
{
	if (m_pBitmap)
	{
		delete m_pBitmap;
		m_pBitmap = NULL;
	}
}

bool PngImage::GetResource(LPCTSTR lpName, LPCTSTR lpType, void* pResource, int& nBufSize)
{
	HRSRC		hResInfo;
	HANDLE		hRes;
	LPSTR		lpRes = NULL;
	int			nLen = 0;
	bool		bResult = FALSE;

	// Find the resource

	hResInfo = FindResource(nullptr, lpName, lpType);
	if (hResInfo == NULL)
	{
		DWORD dwErr = GetLastError();
		return false;
	}

	// Load the resource
	hRes = LoadResource(nullptr, hResInfo);

	if (hRes == NULL)
		return false;

	// Lock the resource
	lpRes = (char*)LockResource(hRes);

	if (lpRes != NULL)
	{
		if (pResource == NULL)
		{
			nBufSize = SizeofResource(nullptr, hResInfo);
			bResult = true;
		}
		else
		{
			if (nBufSize >= (int)SizeofResource(nullptr, hResInfo))
			{
				memcpy(pResource, lpRes, nBufSize);
				bResult = true;
			}
		}

		UnlockResource(hRes);
	}

	// Free the resource
	FreeResource(hRes);

	return bResult;
}

bool PngImage::LoadFromBuffer(BYTE* pBuff, int nSize)
{
	bool bResult = false;

	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, nSize);
	if (hGlobal)
	{
		void* pData = GlobalLock(hGlobal);
		if (pData)
		{
			memcpy(pData, pBuff, nSize);
		}

		GlobalUnlock(hGlobal);
		
		if (CreateStreamOnHGlobal(hGlobal, TRUE, &m_pStream) == S_OK)
		{
			bResult = true;
		}
	}

	return bResult;
}
