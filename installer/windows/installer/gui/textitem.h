#pragma once
#include <Windows.h>
#include <gdiplus.h>
#include <string>

class TextItem
{
public:
	TextItem(const wchar_t *text, int size, bool isBold);

	const wchar_t *text() const;
	int textWidth() const;
	int textHeight() const;
	Gdiplus::Font *getFont();

	void changeText(const wchar_t *text);

private:
	std::wstring text_;
	int width_;
	int height_;
	int size_;
	bool isBold_;
};