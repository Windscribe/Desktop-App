#include "ImageResources.h"
#include <time.h>
#include "dpiscale.h"
#include "../../../Utils/utils.h"
// #include <Windowsx.h>

extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;

ImageResources::ImageResources() : forwardArrow_(NULL), badgeIcon_(NULL), closeIcon_(NULL),
	minimizeIcon_(NULL), settingsIcon_(NULL), folderIcon_(NULL), checkIcon_(NULL),
	greenToggleBg_(NULL), whiteToggleBg_(NULL), toggleButtonBlack_(NULL), backgroundBrush_(NULL), 
	backgroundImage_(NULL)
{

}

ImageResources::~ImageResources()
{
	SAFE_DELETE(forwardArrow_);
	SAFE_DELETE(badgeIcon_);
	SAFE_DELETE(closeIcon_);
	SAFE_DELETE(minimizeIcon_);
	SAFE_DELETE(settingsIcon_);
	SAFE_DELETE(folderIcon_);
	SAFE_DELETE(checkIcon_);
	SAFE_DELETE(greenToggleBg_);
	SAFE_DELETE(whiteToggleBg_);
	SAFE_DELETE(toggleButtonBlack_);
	SAFE_DELETE(backgroundBrush_);
	SAFE_DELETE(backgroundImage_);
}

bool ImageResources::init()
{
	forwardArrow_ = new SVGImage(L"FRWRD_ARROW_ICON", (float)SCALE_FACTOR);
	badgeIcon_ = new SVGImage(L"BADGE_ICON", (float)SCALE_FACTOR);
	closeIcon_ = new SVGImage(L"WINDOWS_CLOSE_ICON", (float)SCALE_FACTOR);
	minimizeIcon_ = new SVGImage(L"WINDOWS_MINIMIZE_ICON", (float)SCALE_FACTOR);
	settingsIcon_ = new SVGImage(L"SETTINGS_ICON", (float)SCALE_FACTOR);
	folderIcon_ = new SVGImage(L"FOLDER_ICON", (float)SCALE_FACTOR);
    checkIcon_ = new SVGImage(L"OK_BUTTON_ICON", (float)SCALE_FACTOR);

	greenToggleBg_ = new SVGImage(L"GREEN_TOGGLE_BG", (float)SCALE_FACTOR);
	whiteToggleBg_ = new SVGImage(L"WHITE_TOGGLE_BG", (float)SCALE_FACTOR);
	toggleButtonBlack_ = new SVGImage(L"TOGGLE_BUTTON_BLACK", (float)SCALE_FACTOR);

	backgroundBrush_ = new Gdiplus::SolidBrush(Gdiplus::Color(3, 9, 28));


	struct ImageDescr {
		std::wstring resourceName_;
		int size_;
	};

	std::vector<ImageDescr> backgroundImages{ {L"WS_BACKGROUND_IMAGE", 350 }, {L"WS_BACKGROUND_IMAGE15", 525 }, {L"WS_BACKGROUND_IMAGE2", 700 }, {L"WS_BACKGROUND_IMAGE3", 1050 } };


	bool bFound = false;
	for (const ImageDescr &id : backgroundImages)
	{
		if (WINDOW_WIDTH <= id.size_)
		{
			backgroundImage_ = new PngImage(id.resourceName_, WINDOW_WIDTH, WINDOW_HEIGHT);
			bFound = true;
			break;
		}
	}
	if (backgroundImage_ == nullptr)
	{
		backgroundImage_ = new PngImage(L"WS_BACKGROUND_IMAGE3", WINDOW_WIDTH, WINDOW_HEIGHT);
	}

	return true;
}