#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "svg/svgimage.h"
#include "png/pngimage.h"

extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;

class ImageResources
{
public:
	ImageResources();
	virtual ~ImageResources();
	bool init();

	Gdiplus::Image* getForwardArrow() { return forwardArrow_->getImage(); }
	Gdiplus::Image* getBadgeIcon() { return badgeIcon_->getImage(); }

	Gdiplus::Image* getCloseIcon() { return closeIcon_->getImage(); }
	Gdiplus::Image* getMinimizeIcon() { return minimizeIcon_->getImage(); }
	Gdiplus::Image* getSettingsIcon() { return settingsIcon_->getImage(); }
	Gdiplus::Image* getFolderIcon() { return folderIcon_->getImage(); }
	Gdiplus::Image* getCheckIcon() { return checkIcon_->getImage(); }

	Gdiplus::Image* getGreenToggleBg() { return greenToggleBg_->getImage(); }
	Gdiplus::Image* getWhiteToggleBg() { return whiteToggleBg_->getImage(); }
	Gdiplus::Image* getToggleButtonBlack() { return toggleButtonBlack_->getImage(); }

	Gdiplus::SolidBrush* getBackgroundBrush() { return backgroundBrush_; }
	PngImage* getBackgroundImage() { return backgroundImage_; }

private:
	SVGImage *forwardArrow_;
	SVGImage *badgeIcon_;
	SVGImage *closeIcon_;
	SVGImage *minimizeIcon_;
	SVGImage *settingsIcon_;
	SVGImage *folderIcon_;
	SVGImage *checkIcon_;
	SVGImage *greenToggleBg_;
	SVGImage *whiteToggleBg_;
	SVGImage *toggleButtonBlack_;

	PngImage *backgroundImage_;
	Gdiplus::SolidBrush *backgroundBrush_;
};
