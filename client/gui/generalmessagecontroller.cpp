#include "generalmessagecontroller.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

GeneralMessageController::GeneralMessageController() : controller_(nullptr), isShowing_(false)
{
}

void GeneralMessageController::setMainWindowController(MainWindowController *controller) {
    controller_ = controller;

    connect(dynamic_cast<QObject*>(controller->getGeneralMessageWindow()), SIGNAL(acceptClick()), SLOT(onAcceptClick()));
    connect(dynamic_cast<QObject*>(controller->getGeneralMessageWindow()), SIGNAL(rejectClick()), SLOT(onRejectClick()));
    connect(dynamic_cast<QObject*>(controller->getGeneralMessageWindow()), SIGNAL(tertiaryClick()), SLOT(onTertiaryClick()));
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

    MainWindowController::WINDOW_ID source = controller_->currentWindow();
    if (!(flags & GeneralMessage::kNoWindowChange) && source == MainWindowController::WINDOW_ID_GENERAL_MESSAGE) {
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
    showMessage(new GeneralMessage(icon, title, desc, acceptText, rejectText, tertiaryText, acceptFunc, rejectFunc, tertiaryFunc, source, flags, learnMoreUrl));
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
    IGeneralMessageWindow *window = controller_->getGeneralMessageWindow();
    GeneralMessage *message = messages_.first();
    if (message == nullptr) {
        // should not be here
        WS_ASSERT(false);
        return;
    }

    window->setIcon(message->icon);
    window->setTitle(message->title);
    window->setDescription(message->desc);
    window->setAcceptText(message->acceptText);
    window->setTertiaryText(message->tertiaryText);
    window->setRejectText(message->rejectText);
    if (message->flags & GeneralMessage::kShowBottomPanel) {
        window->setShowBottomPanel(true);
        window->setLearnMoreUrl(message->learnMoreUrl);
    } else {
        window->setShowBottomPanel(false);
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

    IGeneralMessageWindow *window = controller_->getGeneralMessageWindow();
    bool remember = window->isRememberChecked();

    if (res == ACCEPT && message->acceptFunc) {
        message->acceptFunc(remember);
    } else if (res == TERTIARY && message->tertiaryFunc) {
        message->tertiaryFunc(remember);
    } else if (res == REJECT && message->rejectFunc) {
        message->rejectFunc(remember);
    }

    if (!messages_.isEmpty()) {
        showNext();
    } else {
        isShowing_ = false;
        if (!(message->flags & GeneralMessage::kNoWindowChange)) {
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
    }

    SAFE_DELETE(message);
}
