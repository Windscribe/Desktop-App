#ifndef EXTERNALCONFIGWINDOWITEM_H
#define EXTERNALCONFIGWINDOWITEM_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "../backend/backend.h"
#include "commongraphics/bubblebutton.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"
#include "externalconfig/iexternalconfigwindow.h"

namespace ExternalConfigWindow {

class ExternalConfigWindowItem : public ScalableGraphicsObject, public IExternalConfigWindow
{
    Q_OBJECT
    Q_INTERFACES(IExternalConfigWindow)
public:
    explicit ExternalConfigWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);

    QGraphicsObject *getGraphicsObject() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setClickable(bool isClickable) override;
    void updateScaling() override;

    void setIcon(QString iconPath);
    void setButtonText(QString text);

signals:
    void buttonClick() override;
    void escapeClick() override;
    void minimizeClick() override;
    void closeClick() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onEscClicked();
    void onButtonClicked();

    void onBackgroundOpacityChange(const QVariant &value);
    void onTextOpacityChange(const QVariant &value);
    void onEscTextOpacityChange(const QVariant &value);
    void onDockedModeChanged(bool bIsDockedToTray);
    void onLanguageChanged();

private:
    CommonGraphics::EscapeButton *escButton_;
    CommonGraphics::BubbleButton *okButton_;

    IconButton *closeButton_;
    IconButton *minimizeButton_;

    double curEscTextOpacity_;
    double curBackgroundOpacity_;
    double curTextOpacity_;

    QString curIconPath_;

    // constants:
    static constexpr int ICON_POS_Y = 48;

    static constexpr int TITLE_POS_Y = 104;
    static constexpr int DESCRIPTION_POS_Y = 140;
    static constexpr int DESCRIPTION_WIDTH_MIN = 218;

    static constexpr int CONNECT_BUTTON_POS_X = 122;
    static constexpr int CONNECT_BUTTON_POS_Y = 218;

    void updatePositions();
};

}
#endif // EXTERNALCONFIGWINDOWITEM_H
