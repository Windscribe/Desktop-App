#ifndef APPVERSION_H
#define APPVERSION_H

#include <QString>

class AppVersion
{
public:
    static AppVersion &instance()
    {
        static AppVersion s;
        return s;
    }

    QString version() const;
    QString build() const;
    QString getFullString() const;

private:
    AppVersion();

    QString version_;
    QString build_;

    int major_;
    int minor_;
    int buildInt_;

    bool isBeta_;
};

#endif // APPVERSION_H
