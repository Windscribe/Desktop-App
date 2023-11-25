#pragma once

#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/preferences.h"
#include "commongraphics/basepage.h"
#include "preferenceswindow/preferencegroup.h"
#include "preferenceswindow/linkitem.h"

namespace PreferencesWindow {

class HelpWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit HelpWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const override;
    void setSendLogResult(bool success);

signals:
    void viewLogClick();
    void sendLogClick();

private slots:
    void onSendLogClick();
    void onLanguageChanged();

private:
    enum SEND_LOG_STATE {
        NOT_SENT,
        SENDING,
        SENT,
        FAILED
    };
    SEND_LOG_STATE sendLogState_;

    PreferenceGroup *knowledgeBaseGroup_;
    LinkItem *knowledgeBaseItem_;
    PreferenceGroup *talkToGarryGroup_;
    LinkItem *talkToGarryItem_;
    PreferenceGroup *sendTicketGroup_;
    LinkItem *sendTicketItem_;
    PreferenceGroup *communitySupportGroup_;
    LinkItem *communitySupportItem_;
    LinkItem *redditItem_;
    LinkItem *discordItem_;
    PreferenceGroup *viewLogGroup_;
    LinkItem *viewLogItem_;
    PreferenceGroup *sendLogGroup_;
    LinkItem *sendLogItem_;
};

} // namespace PreferencesWindow
