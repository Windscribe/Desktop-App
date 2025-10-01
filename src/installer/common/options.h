#pragma once

#include <QString>

typedef struct InstallerOptions
{
    int centerX;
    int centerY;
    bool updating;
    bool autostart;
    bool silent;
    bool factoryReset;
    QString installPath;
    QString username;
    QString password;

    InstallerOptions()
        : centerX(-1), centerY(-1), updating(false), autostart(true), silent(false),
          factoryReset(false), installPath(""), username(""), password("")
    {
    }
} InstallerOptions;
