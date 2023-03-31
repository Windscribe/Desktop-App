#pragma once

#include "commongraphics/resizablewindow.h"
#include "commongraphics/scalablegraphicsobject.h"

class IGeneralMessageWindow : public ResizableWindow
{
public:
    enum Style { kBright, kDark };
    enum Shape { kConnectScreenAlphaShape, kConnectScreenVanGoghShape, kLoginScreenShape };

    explicit IGeneralMessageWindow(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
        : ResizableWindow(parent, preferences, preferencesHelper) {}
    virtual ~IGeneralMessageWindow() {}

    virtual void setTitle(const QString &title) = 0;
    virtual void setIcon(const QString &icon) = 0;
    virtual void setDescription(const QString &desc) = 0;
    virtual void setAcceptText(const QString &text) = 0;
    virtual void setRejectText(const QString &text) = 0;
    virtual void setTitleSize(int size) = 0;
    virtual void setBackgroundShape(Shape shape) = 0;
    virtual void setSpinnerMode(bool on) = 0;

signals:
    virtual void acceptClick() = 0;
    virtual void rejectClick() = 0;
};

Q_DECLARE_INTERFACE(IGeneralMessageWindow, "IGeneralMessageWindow")