#include "helpwindowitem.h"
#include "utils/hardcodedsettings.h"
#include "graphicresources/imageresourcessvg.h"

namespace PreferencesWindow {

HelpWindowItem::HelpWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent)
{
    Q_UNUSED(preferences);
    Q_UNUSED(preferencesHelper);

    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);
    
    knowledgeBaseGroup_ = new PreferenceGroup(this, "All you need to know about Windscribe.");
    knowledgeBaseItem_ = new LinkItem(knowledgeBaseGroup_,
                                      LinkItem::LinkType::EXTERNAL_LINK,
                                      tr("Knowledge Base"),
                                      QString("https://%1/support/knowledgebase").arg(HardcodedSettings::instance().serverUrl()));
    knowledgeBaseItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/KNOWLEDGE_BASE"));
    knowledgeBaseGroup_->addItem(knowledgeBaseItem_);
    addItem(knowledgeBaseGroup_);

    talkToGarryGroup_ = new PreferenceGroup(this, "Not as smart as a human, but can still answer your questions.");
    talkToGarryItem_ = new LinkItem(talkToGarryGroup_,
                                    LinkItem::LinkType::EXTERNAL_LINK,
                                    tr("Talk to Garry"),
                                    QString("https://%1/support?garry=1").arg(HardcodedSettings::instance().serverUrl()));
    talkToGarryItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/TALK_TO_GARRY"));
    talkToGarryGroup_->addItem(talkToGarryItem_);
    addItem(talkToGarryGroup_);

    sendTicketGroup_ = new PreferenceGroup(this, "Stuck? Send us a ticket.");
    sendTicketItem_ = new LinkItem(sendTicketGroup_,
                                   LinkItem::LinkType::EXTERNAL_LINK,
                                   tr("Send Ticket"),
                                   QString("https://%1/support/ticket").arg(HardcodedSettings::instance().serverUrl()));
    sendTicketItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SEND_TICKET"));
    sendTicketGroup_->addItem(sendTicketItem_);
    addItem(sendTicketGroup_);

    communitySupportGroup_ = new PreferenceGroup(this, "Best places to help and get help from other users.");
    communitySupportItem_ = new LinkItem(communitySupportGroup_, LinkItem::LinkType::TEXT_ONLY, tr("Community Support"));
    communitySupportItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/COMMUNITY_SUPPORT"));
    communitySupportGroup_->addItem(communitySupportItem_);
    redditItem_ = new LinkItem(communitySupportGroup_, LinkItem::LinkType::EXTERNAL_LINK, tr("Reddit"), "https://www.reddit.com/r/Windscribe/");
    communitySupportGroup_->addItem(redditItem_);
    discordItem_ = new LinkItem(communitySupportGroup_, LinkItem::LinkType::EXTERNAL_LINK, tr("Discord"), "https://discord.gg/vpn");
    communitySupportGroup_->addItem(discordItem_);
    addItem(communitySupportGroup_);

    viewLogGroup_ = new PreferenceGroup(this);
    viewLogItem_ = new LinkItem(viewLogGroup_, LinkItem::LinkType::BLANK_LINK, tr("View Debug Log"));
    viewLogItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/VIEW_LOG"));
    connect(viewLogItem_, &LinkItem::clicked, this, &HelpWindowItem::viewLogClick);
    viewLogGroup_->addItem(viewLogItem_);
    addItem(viewLogGroup_);

    sendLogGroup_ = new PreferenceGroup(this);
    sendLogItem_ = new LinkItem(sendLogGroup_, LinkItem::LinkType::BLANK_LINK, tr("Send Debug Log"));
    sendLogItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SEND_LOG"));
    connect(sendLogItem_, &LinkItem::clicked, this, &HelpWindowItem::sendLogClick);
    sendLogGroup_->addItem(sendLogItem_);
    addItem(sendLogGroup_);
}

QString HelpWindowItem::caption() const
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Help");
}

void HelpWindowItem::setSendLogResult(bool success)
{
    QString text = tr("Send Debug Log");
    sendLogItem_->setTitle(text);
    QString linkText = tr("Sent, thanks!");
    if (!success)
    {
        linkText = tr("Failed!");
    }
    sendLogItem_->setLinkText(linkText);
}

void HelpWindowItem::onSendLogClick()
{
    QString text = tr("Sending log...");
    sendLogItem_->setTitle(text);
    emit sendLogClick();
}

} // namespace PreferencesWindow