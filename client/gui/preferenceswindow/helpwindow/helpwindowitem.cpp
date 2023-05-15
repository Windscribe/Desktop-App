#include "helpwindowitem.h"
#include "languagecontroller.h"
#include "utils/hardcodedsettings.h"
#include "graphicresources/imageresourcessvg.h"

namespace PreferencesWindow {

HelpWindowItem::HelpWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent), sendLogState_(NOT_SENT)
{
    Q_UNUSED(preferences);
    Q_UNUSED(preferencesHelper);

    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);
    
    knowledgeBaseGroup_ = new PreferenceGroup(this);
    knowledgeBaseItem_ = new LinkItem(knowledgeBaseGroup_,
                                      LinkItem::LinkType::EXTERNAL_LINK,
                                      "",
                                      QString("https://%1/support/knowledgebase").arg(HardcodedSettings::instance().serverUrl()));
    knowledgeBaseItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/KNOWLEDGE_BASE"));
    knowledgeBaseGroup_->addItem(knowledgeBaseItem_);
    addItem(knowledgeBaseGroup_);

    talkToGarryGroup_ = new PreferenceGroup(this);
    talkToGarryItem_ = new LinkItem(talkToGarryGroup_,
                                    LinkItem::LinkType::EXTERNAL_LINK,
                                    "",
                                    QString("https://%1/support?garry=1").arg(HardcodedSettings::instance().serverUrl()));
    talkToGarryItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/TALK_TO_GARRY"));
    talkToGarryGroup_->addItem(talkToGarryItem_);
    addItem(talkToGarryGroup_);

    sendTicketGroup_ = new PreferenceGroup(this);
    sendTicketItem_ = new LinkItem(sendTicketGroup_,
                                   LinkItem::LinkType::EXTERNAL_LINK,
                                   "",
                                   QString("https://%1/support/ticket").arg(HardcodedSettings::instance().serverUrl()));
    sendTicketItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SEND_TICKET"));
    sendTicketGroup_->addItem(sendTicketItem_);
    addItem(sendTicketGroup_);

    communitySupportGroup_ = new PreferenceGroup(this);
    communitySupportItem_ = new LinkItem(communitySupportGroup_, LinkItem::LinkType::TEXT_ONLY);
    communitySupportItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/COMMUNITY_SUPPORT"));
    communitySupportGroup_->addItem(communitySupportItem_);
    redditItem_ = new LinkItem(communitySupportGroup_, LinkItem::LinkType::EXTERNAL_LINK, "", "https://www.reddit.com/r/Windscribe/");
    communitySupportGroup_->addItem(redditItem_);
    discordItem_ = new LinkItem(communitySupportGroup_, LinkItem::LinkType::EXTERNAL_LINK, "", "https://discord.gg/vpn");
    communitySupportGroup_->addItem(discordItem_);
    addItem(communitySupportGroup_);

    viewLogGroup_ = new PreferenceGroup(this);
    viewLogItem_ = new LinkItem(viewLogGroup_, LinkItem::LinkType::BLANK_LINK);
    viewLogItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/VIEW_LOG"));
    connect(viewLogItem_, &LinkItem::clicked, this, &HelpWindowItem::viewLogClick);
    viewLogGroup_->addItem(viewLogItem_);
    addItem(viewLogGroup_);

    sendLogGroup_ = new PreferenceGroup(this);
    sendLogItem_ = new LinkItem(sendLogGroup_, LinkItem::LinkType::BLANK_LINK);
    sendLogItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SEND_LOG"));
    connect(sendLogItem_, &LinkItem::clicked, this, &HelpWindowItem::sendLogClick);
    sendLogGroup_->addItem(sendLogItem_);
    addItem(sendLogGroup_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &HelpWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString HelpWindowItem::caption() const
{
    return tr("Help");
}

void HelpWindowItem::setSendLogResult(bool success)
{
    if (success) {
        sendLogState_ = SENT;
    } else {
        sendLogState_ = FAILED;
    }
    onLanguageChanged();
}

void HelpWindowItem::onSendLogClick()
{
    sendLogState_ = SENDING;
    onLanguageChanged();
    emit sendLogClick();
}

void HelpWindowItem::onLanguageChanged()
{
    knowledgeBaseGroup_->setDescription(tr("All you need to know about Windscribe."));
    knowledgeBaseItem_->setTitle(tr("Knowledge Base"));
    talkToGarryGroup_->setDescription(tr("Not as smart as a human, but can still answer your questions."));
    talkToGarryItem_->setTitle(tr("Talk to Garry"));
    sendTicketGroup_->setDescription(tr("Stuck? Send us a ticket."));
    sendTicketItem_->setTitle(tr("Send Ticket"));
    communitySupportGroup_->setDescription(tr("Best places to help and get help from other users."));
    communitySupportItem_->setTitle(tr("Community Support"));
    redditItem_->setTitle(tr("Reddit"));
    discordItem_->setTitle(tr("Discord"));
    viewLogItem_->setTitle(tr("View Debug Log"));

    if (sendLogState_ == NOT_SENT) {
        sendLogItem_->setTitle(tr("Send Debug Log"));
    } else if (sendLogState_ == SENDING) {
        sendLogItem_->setTitle(tr("Sending log..."));
    } else if (sendLogState_ == SENT) {
        sendLogItem_->setTitle(tr("Sent, thanks!"));
    } else if (sendLogState_ == FAILED) {
        sendLogItem_->setTitle(tr("Failed!"));
    }
}

} // namespace PreferencesWindow