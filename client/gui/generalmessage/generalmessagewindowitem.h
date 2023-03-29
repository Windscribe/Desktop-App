#pragma once

#include "igeneralmessagewindow.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "generalmessageitem.h"

namespace GeneralMessageWindow {

class GeneralMessageWindowItem : public IGeneralMessageWindow
{
    Q_OBJECT
    Q_INTERFACES(IGeneralMessageWindow)

public:
    explicit GeneralMessageWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper,
                                      IGeneralMessageWindow::Style style, const QString &icon = "", const QString &title = "",
                                      const QString &desc = "", const QString &acceptText = "", const QString &rejectText = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setIcon(const QString &icon) override;
    void setTitle(const QString &title) override;
    void setDescription(const QString &desc) override;

    void setAcceptText(const QString &text) override;
    void setRejectText(const QString &text) override;

    void setTitleSize(int size) override;
    void setBackgroundShape(IGeneralMessageWindow::Shape shape) override;

    void setSpinnerMode(bool on) override;

signals:
    void acceptClick() override;
    void rejectClick() override;

protected slots:
    void onAppSkinChanged(APP_SKIN s) override;
    void focusInEvent(QFocusEvent *event) override;

protected:
    void updatePositions() override;

private slots:
    void onEscape();
    void onSpinnerRotationChanged(const QVariant &value);
    void onSpinnerRotationFinished();

private:
    bool isSpinnerMode_;
    int curSpinnerRotation_;
    QVariantAnimation spinnerRotationAnimation_;
    static constexpr int kSpinnerSpeed = 500;

    IGeneralMessageWindow::Shape shape_;
    GeneralMessageItem *contentItem_;

    void updateHeight();
};

} // namespace GeneralMessageWindow
