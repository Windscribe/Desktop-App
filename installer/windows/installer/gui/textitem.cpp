#include "TextItem.h"
#include "Application.h"

using namespace Gdiplus;

TextItem::TextItem(const wchar_t *text, int size, bool isBold) : size_(size), isBold_(isBold)
{
	changeText(text);
}

const wchar_t *TextItem::text() const
{
	return text_.c_str();
}

int TextItem::textWidth() const
{
	return width_;
}
int TextItem::textHeight() const
{
	return height_;
}
Gdiplus::Font *TextItem::getFont()
{
	return g_application->getFontResources()->getFont(size_, isBold_);
}

void TextItem::changeText(const wchar_t *text)
{
	text_ = text;

	Bitmap tempBmp(100, 100);
	Graphics *graphics = Graphics::FromImage(&tempBmp);

	PointF origin(0.0f, 0.0f);
	RectF boundRect;
	Status s = graphics->MeasureString(text_.c_str(), text_.length(), g_application->getFontResources()->getFont(size_, isBold_), origin, &boundRect);

	width_ = (int)boundRect.Width;
	height_ = (int)boundRect.Height;

	delete graphics;
}