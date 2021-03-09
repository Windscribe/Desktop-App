#ifndef SVGIMAGE_H
#define SVGIMAGE_H

#ifdef _WIN32
#include <Windows.h>
#include <gdiplus.h>
#endif

#include <string>

struct NSVGimage;
struct NSVGrasterizer;

class SVGImage
{
public:
	SVGImage(const std::wstring &resource_name, const float &scale);
	virtual ~SVGImage();

	Gdiplus::Image *getImage();

 private:
  #ifdef _WIN32
  Gdiplus::Image* image_;
  #endif
};

#endif // SVGIMAGE_H
