#pragma once

#include <QObject>
#include <QString>
#include "generalmessage.h"
#include "mainwindowcontroller.h"

class GeneralMessageController: public QObject
{
    Q_OBJECT

public:
    static GeneralMessageController &instance()
    {
        static GeneralMessageController gmc;
        return gmc;
    }

    // must be called to complete init
    void setMainWindowController(MainWindowController *mwc);

    void showMessage(const QString &icon, const QString &title, const QString &desc,
                     const QString &acceptText,
                     const QString &rejectText = "",
                     const QString &tertiaryText = "",
                     std::function<void(bool)> acceptFunc = std::function<void(bool)>(nullptr),
                     std::function<void(bool)> rejectFunc = std::function<void(bool)>(nullptr),
                     std::function<void(bool)> tertiaryFunc = std::function<void(bool)>(nullptr),
                     GeneralMessage::Flags flags = GeneralMessage::Flags::kNone,
                     const QString &learnMoreUrl = "");

    void showCredentialPrompt(const QString &icon, const QString &title, const QString &desc,
                              const QString &username,
                              const QString &acceptText,
                              const QString &rejectText = "",
                              const QString &tertiaryText = "",
                              std::function<void(const QString &, const QString &, bool)> acceptFunc = std::function<void(const QString &, const QString &, bool)>(nullptr),
                              std::function<void(bool)> rejectFunc = std::function<void(bool)>(nullptr),
                              std::function<void(bool)> tertiaryFunc = std::function<void(bool)>(nullptr),
                              GeneralMessage::Flags flags = GeneralMessage::Flags::kNone);

    void showMessage(GeneralMessage *message);
    void showCredentialsPrompt(GeneralMessage *message);
    bool hasMessages() const;

    static inline const char *kOk = QT_TR_NOOP("Ok");
    static inline const char *kYes = QT_TR_NOOP("Yes");
    static inline const char *kNo = QT_TR_NOOP("No");
    static inline const char *kCancel = QT_TR_NOOP("Cancel");

private slots:
    void onAcceptClick();
    void onRejectClick();
    void onTertiaryClick();

private:
    enum Result { NONE, ACCEPT, REJECT, TERTIARY };

    MainWindowController *controller_;
    QList<GeneralMessage *> messages_;
    bool isShowing_;

    explicit GeneralMessageController();
    void showNext();
    void handleResult(Result res);
    MainWindowController::WINDOW_ID getSource();
};
