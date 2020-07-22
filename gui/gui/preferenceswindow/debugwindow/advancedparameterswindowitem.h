#ifndef ADVANCEDPARAMETERSWINDOWITEM_H
#define ADVANCEDPARAMETERSWINDOWITEM_H

#include "PreferencesWindow/basepage.h"
#include "../Backend/Preferences/preferences.h"
#include "../Backend/Preferences/preferenceshelper.h"
#include "PreferencesWindow/DebugWindow/advancedparametersitem.h"

namespace PreferencesWindow {

class AdvancedParametersWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit AdvancedParametersWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption();
    void setHeight(int newHeight);

private slots:
    void onAdvancedParametersPreferencesChanged(const QString &text);
    void onAdvancedParametersChanged(const QString &text);

private:
    Preferences *preferences_;
    AdvancedParametersItem *advancedParametersItem_;

};

}

#endif // ADVANCEDPARAMETERSWINDOWITEM_H
