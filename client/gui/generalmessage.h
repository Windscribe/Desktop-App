#pragma once

#include <QObject>
#include <QString>
#include <functional>
#include "mainwindowcontroller.h"

class GeneralMessage
{
public:
    enum Flags {
        kNone = 0,
        kNoWindowChange = 1,
        kFromPreferences = 2,
        kShowBottomPanel = 4,
    };

    explicit GeneralMessage(const QString &icon, const QString &title, const QString &desc, const QString &acceptText,
                            const QString &rejectText, const QString &tertiaryText,
                            std::function<void(bool)> acceptFunc, std::function<void(bool)> rejectFunc,
                            std::function<void(bool)> tertiaryFunc, MainWindowController::WINDOW_ID source, Flags flags,
                            const QString &learnMoreUrl)
    {

        this->icon = icon;
        this->title = title;
        this->desc = desc;
        this->acceptText = acceptText;
        this->rejectText = rejectText;
        this->tertiaryText = tertiaryText;
        this->acceptFunc = acceptFunc;
        this->rejectFunc = rejectFunc;
        this->tertiaryFunc = tertiaryFunc;
        this->source = source;
        this->flags = flags;
        this->learnMoreUrl = learnMoreUrl;
    }

    QString icon;
    QString title;
    QString desc;
    QString acceptText;
    QString rejectText;
    QString tertiaryText;
    std::function<void(bool)> acceptFunc;
    std::function<void(bool)> rejectFunc;
    std::function<void(bool)> tertiaryFunc;
    MainWindowController::WINDOW_ID source;
    Flags flags;
    QString learnMoreUrl;

    static inline const char *kOk = QT_TR_NOOP("Ok");
    static inline const char *kYes = QT_TR_NOOP("Yes");
    static inline const char *kNo = QT_TR_NOOP("No");
    static inline const char *kCancel = QT_TR_NOOP("Cancel");
};