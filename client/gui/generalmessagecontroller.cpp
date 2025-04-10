#include "generalmessagecontroller.h"

#include <QApplication>
#include "utils/utils.h"
#include "utils/ws_assert.h"

GeneralMessageController::GeneralMessageController() : controller_(nullptr), isShowing_(false)
{
}

void GeneralMessageController::setMainWindowController(MainWindowController *controller) {
    controller_ = controller;

    connect(controller->getGeneralMessageWindow(), &GeneralMessageWindow::GeneralMessageWindowItem::acceptClick, this, &GeneralMessageController::onAcceptClick);
    connect(controller->getGeneralMessageWindow(), &GeneralMessageWindow::GeneralMessageWindowItem::rejectClick, this, &GeneralMessageController::onRejectClick);
    connect(controller->getGeneralMessageWindow(), &GeneralMessageWindow::GeneralMessageWindowItem::tertiaryClick, this, &GeneralMessageController::onTertiaryClick);
}

MainWindowController::WINDOW_ID GeneralMessageController::getSource()
{
    MainWindowController::WINDOW_ID source = controller_->currentWindowAfterAnimation();
    if (source == MainWindowController::WINDOW_ID_LOGOUT || source == MainWindowController::WINDOW_ID_EXIT) {
        source = controller_->windowBeforeExit();
    }
    if (source == MainWindowController::WINDOW_ID_GENERAL_MESSAGE) {
        // Look backwards in the message queue until we find something that has a different source
        for (auto it = messages_.rbegin(); it != messages_.rend(); ++it) {
            if ((*it)->source != MainWindowController::WINDOW_ID_GENERAL_MESSAGE) {
                source = (*it)->source;
                break;
            }
        }
        // Should never happen, but if somehow we didn't find any, default to connect screen
        if (source == MainWindowController::WINDOW_ID_GENERAL_MESSAGE) {
            source = MainWindowController::WINDOW_ID_CONNECT;
        }
    }
    return source;
}

void GeneralMessageController::showMessage(const QString &icon, const QString &title, const QString &desc, const QString &acceptText,
                                           const QString &rejectText, const QString &tertiaryText, std::function<void(bool)> acceptFunc,
                                           std::function<void(bool)> rejectFunc, std::function<void(bool)> tertiaryFunc,
                                           GeneralMessage::Flags flags, const QString &learnMoreUrl)
{
    if (controller_ == nullptr) {
        // not initialized
        return;
    }

    showMessage(new GeneralMessage(icon, title, desc, acceptText, rejectText, tertiaryText, acceptFunc, rejectFunc, tertiaryFunc, getSource(), flags, learnMoreUrl));
}

void GeneralMessageController::showCredentialPrompt(const QString &icon, const QString &title, const QString &desc, const QString &username,
                                                    const QString &acceptText, const QString &rejectText, const QString &tertiaryText,
                                                    std::function<void(const QString &, const QString &, bool)> acceptFunc,
                                                    std::function<void(bool)> rejectFunc, std::function<void(bool)> tertiaryFunc,
                                                    GeneralMessage::Flags flags)
{
    if (controller_ == nullptr) {
        // not initialized
        return;
    }

    showMessage(new GeneralMessage(icon, title, desc, username, acceptText, rejectText, tertiaryText, acceptFunc, rejectFunc, tertiaryFunc, getSource(), flags));
}

void GeneralMessageController::showMessage(GeneralMessage *message)
{
    if (controller_ == nullptr) {
        // not initialized
        return;
    }

    for (auto msg : messages_) {
        if (msg->desc == message->desc) {
            // Message already displaying or in queue, ignore
            return;
        }
    }

    messages_.append(message);
    if (!isShowing_) {
        showNext();
    }
}

void GeneralMessageController::showNext()
{
    isShowing_ = true;
    qApp->alert(static_cast<QWidget *>(controller_->parent()));
    GeneralMessageWindow::GeneralMessageWindowItem *window = controller_->getGeneralMessageWindow();
    GeneralMessage *message = messages_.first();
    if (message == nullptr) {
        // should not be here
        WS_ASSERT(false);
        return;
    }

    if (message->type == GeneralMessage::Type::kCredentialPrompt) {
        window->setStyle(GeneralMessageWindow::Style::kDark);
    } else {
        window->setStyle(GeneralMessageWindow::Style::kBright);
    }

    window->setIcon(message->icon);
    window->setTitle(message->title);
    window->setDescription(message->desc);
    if (message->type == GeneralMessage::Type::kCredentialPrompt) {
        window->setShowUsername(!(message->flags & GeneralMessage::kNoUsername));
        window->setShowPassword(true);
        window->setAcceptText(message->acceptText, true);
        window->setUsername(message->username);
    } else {
        window->setShowUsername(false);
        window->setShowPassword(false);
        window->setAcceptText(message->acceptText);
    }
    window->setTertiaryText(message->tertiaryText);
    window->setRejectText(message->rejectText);
    if (message->type == GeneralMessage::Type::kAlert) {
        if (message->flags & GeneralMessage::kShowBottomPanel) {
            window->setShowBottomPanel(true);
            window->setLearnMoreUrl(message->learnMoreUrl);
        } else {
            window->setShowBottomPanel(false);
        }
    }
    controller_->changeWindow(MainWindowController::WINDOW_ID_GENERAL_MESSAGE);
}

void GeneralMessageController::onAcceptClick()
{
    handleResult(ACCEPT);
}

void GeneralMessageController::onRejectClick()
{
    handleResult(REJECT);
}

void GeneralMessageController::onTertiaryClick()
{
    handleResult(TERTIARY);
}

void GeneralMessageController::handleResult(Result res)
{
    if (messages_.isEmpty()) {
        // We should not be here
        WS_ASSERT(false);
        return;
    }

    GeneralMessage *message = messages_.first();
    messages_.removeFirst();

    GeneralMessageWindow::GeneralMessageWindowItem *window = controller_->getGeneralMessageWindow();
    bool remember = window->isRememberChecked();

    MainWindowController::WINDOW_ID curWin = controller_->currentWindowAfterAnimation();
    if (res == ACCEPT) {
        if (message->acceptFunc) {
            message->acceptFunc(remember);
        } else if (message->acceptCredentialFunc) {
            message->acceptCredentialFunc(window->username(), window->password(), remember);
        }
    } else if (res == TERTIARY && message->tertiaryFunc) {
        message->tertiaryFunc(remember);
    } else if (res == REJECT && message->rejectFunc) {
        message->rejectFunc(remember);
    }

    window->clear();

    if (curWin != controller_->currentWindowAfterAnimation()) {
        // The handler function made a window transition, respect it and go to this window instead
        message->source = controller_->currentWindowAfterAnimation();
    }

    if (!messages_.isEmpty()) {
        showNext();
    } else {
        isShowing_ = false;
        // If transitioning to connect window + preferences, use the special id instead
        if (message->source == MainWindowController::WINDOW_ID_CONNECT && message->flags & GeneralMessage::kFromPreferences) {
            controller_->changeWindow(MainWindowController::WINDOW_ID_CONNECT_PREFERENCES);
        } else {
            controller_->changeWindow(message->source);
            if (message->flags & GeneralMessage::kFromPreferences) {
                controller_->expandPreferences();
            }
        }
    }

    SAFE_DELETE(message);
}

bool GeneralMessageController::hasMessages() const
{
    return !messages_.isEmpty();
}

void GeneralMessageController::setSource(MainWindowController::WINDOW_ID source)
{
    messages_.last()->source = source;
}
