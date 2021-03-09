#include "svgimage.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

/*#ifndef _WIN32
//It is taken from here https://stackoverflow.com/questions/34641373/how-do-i-embed-the-contents-of-a-binary-file-in-an-executable-on-mac-os-x
asm(
 ".global _data_start_somefile0\n\t"
 ".global _data_end_somefile0\n\t"
 "_data_start_somefile0:\n\t"
 ".incbin \"resources/0.svg\"\n\t"
 "_data_end_somefile0:\n\t"
   );

extern char data_start_somefile0, data_end_somefile0;
#endif*/

using namespace std;

SVGImage::SVGImage(const wstring &resource_name, const float &scale) : image_(NULL)
{
 #ifdef _WIN32
	HRSRC hResource = FindResource(nullptr,resource_name.c_str(), RT_RCDATA);
	assert(hResource != NULL);

	HGLOBAL hGlobal = LoadResource(nullptr, hResource);
	assert(hGlobal != NULL);

	char *pResourceData = static_cast<char*>(LockResource(hGlobal));
	assert(pResourceData != NULL);
    
    DWORD imageSize = SizeofResource(nullptr, hResource);
#else
    pResourceData = &data_start_somefile0;
    imageSize = int(&data_end_somefile0-&data_start_somefile0);
#endif

    char *tempBuf = new char[imageSize + 1];
    memcpy(tempBuf, pResourceData, imageSize);
	tempBuf[imageSize] = '\0';	// Must be null terminated.

	NSVGimage *nsvgimage = nsvgParse(tempBuf, "px", 96.0f);
	assert(nsvgimage != NULL);

	delete[] tempBuf;

    int w = static_cast<int>(nsvgimage->width);
    int h = static_cast<int>(nsvgimage->height);

	NSVGrasterizer *rast = nsvgCreateRasterizer();
	assert(rast != NULL);

    unsigned char *img = new unsigned char[static_cast<size_t>(w * h * 4 * scale * scale)];
    nsvgRasterize(rast, nsvgimage, 0, 0, scale, img, static_cast<int>(w*scale), static_cast<int>(h*scale), static_cast<int>(scale*w*4));
    int len;
	unsigned char *png = stbi_write_png_to_mem(static_cast<unsigned char *>(img), static_cast<int>(scale*w*4), static_cast<int>(w*scale), static_cast<int>(h*scale), 4, &len);

#ifdef _WIN32
    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, static_cast<size_t>(len));
    if (hBuffer)
    {
		void *pBuffer = GlobalLock(hBuffer);
		if (pBuffer)
		{
			CopyMemory(pBuffer, png, len);

			IStream* pStream = nullptr;
			if (::CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK)
            {
				image_ = new Gdiplus::Image(pStream, false);
            }
			GlobalUnlock(pBuffer);
		}
		GlobalFree(hBuffer);
    }
	else
	{
		assert(false);
	}

	free(png);
	delete[] img;
	nsvgDeleteRasterizer(rast);
	nsvgDelete(nsvgimage);
#endif
}

SVGImage::~SVGImage()
{
	if (image_)
	{
		delete image_;
	}
}

Gdiplus::Image *SVGImage::getImage()
{
	return image_;
}
