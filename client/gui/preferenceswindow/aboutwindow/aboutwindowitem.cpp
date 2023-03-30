#include "aboutwindowitem.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/hardcodedsettings.h"

namespace PreferencesWindow {

AboutWindowItem::AboutWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent)
{
    Q_UNUSED(preferences);
    Q_UNUSED(preferencesHelper);

    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);
    
    group_ = new PreferenceGroup(this, "", "");

    LinkItem *item1 = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, tr("Status"),
                                   QString("https://%1/status").arg(HardcodedSettings::instance().serverUrl()));
    group_->addItem(item1);
    LinkItem *item2 = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, tr("About Us"),
                                   QString("https://%1/about").arg(HardcodedSettings::instance().serverUrl()));
    group_->addItem(item2);
    LinkItem *item3 = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, tr("Privacy Policy"),
                                   QString("https://%1/privacy").arg(HardcodedSettings::instance().serverUrl()));
    group_->addItem(item3);
    LinkItem *item4 = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, tr("Terms"),
                                   QString("https://%1/terms").arg(HardcodedSettings::instance().serverUrl()));
    group_->addItem(item4);
    LinkItem *item5 = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, tr("Blog"),
                                   "https://blog.windscribe.com");
    group_->addItem(item5);
    LinkItem *item6 = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, tr("Jobs"),
                                   "https://angel.co/company/windscribe");
    group_->addItem(item6);
    LinkItem *item7 = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, tr("Software Licenses"),
                                   QString("https://%1/terms/oss").arg(HardcodedSettings::instance().serverUrl()));
    group_->addItem(item7);

    addItem(group_);
}

QString AboutWindowItem::caption() const
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "About");
}

} // namespace PreferencesWindow