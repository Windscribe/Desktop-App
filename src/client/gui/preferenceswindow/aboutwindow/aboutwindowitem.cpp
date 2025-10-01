#include "aboutwindowitem.h"
#include "languagecontroller.h"
#include "preferenceswindow/preferencesconst.h"
#include "utils/hardcodedsettings.h"

namespace PreferencesWindow {

AboutWindowItem::AboutWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent)
{
    Q_UNUSED(preferences);
    Q_UNUSED(preferencesHelper);

    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    group_ = new PreferenceGroup(this, "", "");

    statusLink_ = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, "",
                               QString("https://%1/status").arg(HardcodedSettings::instance().windscribeServerUrl()));
    group_->addItem(statusLink_);

    aboutUsLink_ = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, "",
                                QString("https://%1/about").arg(HardcodedSettings::instance().windscribeServerUrl()));
    group_->addItem(aboutUsLink_);

    privacyLink_ = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, "",
                                QString("https://%1/privacy").arg(HardcodedSettings::instance().windscribeServerUrl()));
    group_->addItem(privacyLink_);

    termsLink_ = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, "",
                              QString("https://%1/terms").arg(HardcodedSettings::instance().windscribeServerUrl()));
    group_->addItem(termsLink_);

    blogLink_ = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, "",
                             "https://blog.windscribe.com");
    group_->addItem(blogLink_);

    jobsLink_ = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, "",
                             "https://angel.co/company/windscribe");
    group_->addItem(jobsLink_);

    licensesLink_ = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, "",
                                 QString("https://%1/terms/oss").arg(HardcodedSettings::instance().windscribeServerUrl()));
    group_->addItem(licensesLink_);

    changelogLink_ = new LinkItem(group_, LinkItem::LinkType::EXTERNAL_LINK, "",
                                  QString("https://%1/changelog").arg(HardcodedSettings::instance().windscribeServerUrl()));
    group_->addItem(changelogLink_);

    addItem(group_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &AboutWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString AboutWindowItem::caption() const
{
    return tr("About");
}

void AboutWindowItem::onLanguageChanged()
{
    statusLink_->setTitle(tr("Status"));
    aboutUsLink_->setTitle(tr("About Us"));
    privacyLink_->setTitle(tr("Privacy Policy"));
    termsLink_->setTitle(tr("Terms"));
    blogLink_->setTitle(tr("Blog"));
    jobsLink_->setTitle(tr("Jobs"));
    licensesLink_->setTitle(tr("Software Licenses"));
    changelogLink_->setTitle(tr("Changelog"));
}

} // namespace PreferencesWindow
