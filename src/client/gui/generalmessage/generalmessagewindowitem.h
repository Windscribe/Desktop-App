#pragma once

#include "commongraphics/resizablewindow.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "generalmessageitem.h"
#include "generalmessagetypes.h"

namespace GeneralMessageWindow {

class GeneralMessageWindowItem : public ResizableWindow
{
    Q_OBJECT
public:
    explicit GeneralMessageWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper,
                                      GeneralMessageWindow::Style style, const QString &icon = "", const QString &title = "",
                                      const QString &desc = "", const QString &acceptText = "", const QString &rejectText = "",
                                      const QString &tertiaryText = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setStyle(GeneralMessageWindow::Style style);
    void setIcon(const QString &icon);
    void setTitle(const QString &title);
    void setDescription(const QString &desc);

    void setAcceptText(const QString &text, bool showRemember = false);
    void setRejectText(const QString &text);
    void setTertiaryText(const QString &text);

    void setTitleSize(int size);
    void setBackgroundShape(GeneralMessageWindow::Shape shape);

    void setSpinnerMode(bool on);
    void setShowBottomPanel(bool on);
    void setLearnMoreUrl(const QString &url);
    void setShowUsername(bool on);
    void setShowPassword(bool on);
    void clear();

    void setUsername(const QString &username);

    bool isRememberChecked();
    GeneralMessageWindow::Shape backgroundShape() const;
    QString username() const;
    QString password() const;

signals:
    void acceptClick();
    void rejectClick();
    void tertiaryClick();

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

    GeneralMessageWindow::Shape shape_;
    GeneralMessageItem *contentItem_;

    void updateHeight();
};

} // namespace GeneralMessageWindow
