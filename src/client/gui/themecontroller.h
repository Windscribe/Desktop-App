#pragma once

#include <QObject>

class ThemeController : public QObject
{
    Q_OBJECT
public:
    static ThemeController &instance()
    {
        static ThemeController tc;
        return tc;
    }

    bool isOsDarkTheme() const;

signals:
    void osThemeChanged(bool isDarkTheme);

private slots:
    void onOsThemeChanged(const Qt::ColorScheme &colorScheme);

private:
    ThemeController();
    ~ThemeController();

    bool isOsDarkTheme_ = false;
};
