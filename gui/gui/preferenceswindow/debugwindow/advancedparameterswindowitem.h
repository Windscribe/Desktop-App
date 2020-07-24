#ifndef ADVANCEDPARAMETERSWINDOWITEM_H
#define ADVANCEDPARAMETERSWINDOWITEM_H

#include "preferenceswindow/basepage.h"
#include "../backend/preferences/preferences.h"
#include "../backend/preferences/preferenceshelper.h"
#include "preferenceswindow/debugwindow/advancedparametersitem.h"

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
