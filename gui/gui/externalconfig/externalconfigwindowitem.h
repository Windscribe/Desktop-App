#ifndef EXTERNALCONFIGWINDOWITEM_H
#define EXTERNALCONFIGWINDOWITEM_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "commongraphics/bubblebuttondark.h"
#include "commongraphics/iconbutton.h"
#include "externalconfig/iexternalconfigwindow.h"
#include "preferenceswindow/escapebutton.h"

namespace ExternalConfigWindow {

class ExternalConfigWindowItem : public ScalableGraphicsObject, public IExternalConfigWindow
{
    Q_OBJECT
    Q_INTERFACES(IExternalConfigWindow)
public:
    explicit ExternalConfigWindowItem(QGraphicsObject *parent = nullptr);

    virtual QGraphicsObject *getGraphicsObject();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual void setClickable(bool isClickable);
    virtual void updateScaling();

    void setIcon(QString iconPath);
    void setButtonText(QString text);

signals:
    void buttonClick();
    void escapeClick();

    void minimizeClick();
    void closeClick();

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void onEscClicked();
    void onButtonClicked();

    void onBackgroundOpacityChange(const QVariant &value);
    void onTextOpacityChange(const QVariant &value);
    void onEscTextOpacityChange(const QVariant &value);

private:
    PreferencesWindow::EscapeButton *escButton_;
    CommonGraphics::BubbleButtonDark *okButton_;

    IconButton *closeButton_;
    IconButton *minimizeButton_;

    double curEscTextOpacity_;
    double curBackgroundOpacity_;
    double curTextOpacity_;

    QString curIconPath_;

    // constants:
    const int ICON_POS_Y = 48;

    const int TITLE_POS_Y = 104;
    const int DESCRIPTION_POS_Y = 140;
    const int DESCRIPTION_WIDTH_MIN = 218;

    const int CONNECT_BUTTON_POS_X = 122;
    const int CONNECT_BUTTON_POS_Y = 218;

    void updatePositions();
};

}
#endif // EXTERNALCONFIGWINDOWITEM_H
