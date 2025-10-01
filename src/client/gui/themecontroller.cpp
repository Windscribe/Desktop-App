#include "themecontroller.h"

#include <QGuiApplication>
#include <QStyleHints>

ThemeController::ThemeController()
{
    QStyleHints *hints = QGuiApplication::styleHints();
    if (hints != nullptr && hints->colorScheme() == Qt::ColorScheme::Dark) {
        isOsDarkTheme_ = true;
    }

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &ThemeController::onOsThemeChanged);
}

ThemeController::~ThemeController()
{
}

bool ThemeController::isOsDarkTheme() const
{
    return isOsDarkTheme_;
}

void ThemeController::onOsThemeChanged(const Qt::ColorScheme &colorScheme)
{
    isOsDarkTheme_ = colorScheme == Qt::ColorScheme::Dark; // If colorScheme is Unknown, it's light by default
    emit osThemeChanged(isOsDarkTheme_);
}
