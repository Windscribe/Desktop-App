#pragma once

#include "commongraphics/basepage.h"
#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/preferences.h"
#include "../linkitem.h"
#include "../preferencegroup.h"

namespace PreferencesWindow {

class AboutWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit AboutWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const override;

private slots:
    void onLanguageChanged();

private:
    PreferenceGroup *group_;
    LinkItem *statusLink_;
    LinkItem *aboutUsLink_;
    LinkItem *privacyLink_;
    LinkItem *termsLink_;
    LinkItem *blogLink_;
    LinkItem *jobsLink_;
    LinkItem *licensesLink_;
    LinkItem *changelogLink_;
};

} // namespace PreferencesWindow
