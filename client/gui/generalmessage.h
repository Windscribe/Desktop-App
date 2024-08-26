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
        kFromPreferences = 1,
        kShowBottomPanel = 2, // alert only
        kNoUsername = 4,      // credential prompt only
    };
    enum Type {
        kAlert,
        kCredentialPrompt,
    };

    explicit GeneralMessage(const QString &icon, const QString &title, const QString &desc, const QString &acceptText,
                            const QString &rejectText, const QString &tertiaryText,
                            std::function<void(bool)> acceptFunc, std::function<void(bool)> rejectFunc,
                            std::function<void(bool)> tertiaryFunc, MainWindowController::WINDOW_ID source, Flags flags,
                            const QString &learnMoreUrl)
    {
        this->type = Type::kAlert;
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

    explicit GeneralMessage(const QString &icon, const QString &title, const QString &desc, const QString &username,
                            const QString &acceptText, const QString &rejectText, const QString &tertiaryText,
                            std::function<void(const QString &, const QString &, bool)> acceptFunc, std::function<void(bool)> rejectFunc,
                            std::function<void(bool)> tertiaryFunc, MainWindowController::WINDOW_ID source, Flags flags)
    {
        this->type = Type::kCredentialPrompt;
        this->icon = icon;
        this->title = title;
        this->desc = desc;
        this->username = username;
        this->acceptText = acceptText;
        this->rejectText = rejectText;
        this->tertiaryText = tertiaryText;
        this->acceptCredentialFunc = acceptFunc;
        this->rejectFunc = rejectFunc;
        this->tertiaryFunc = tertiaryFunc;
        this->source = source;
        this->flags = flags;
        this->learnMoreUrl = "";
    }

    Type type;
    QString icon;
    QString title;
    QString desc;
    QString acceptText;
    QString rejectText;
    QString tertiaryText;
    std::function<void(bool)> acceptFunc;
    std::function<void(const QString &, const QString &, bool)> acceptCredentialFunc;
    std::function<void(bool)> rejectFunc;
    std::function<void(bool)> tertiaryFunc;
    MainWindowController::WINDOW_ID source;
    Flags flags;
    QString learnMoreUrl;
    QString username;
};
