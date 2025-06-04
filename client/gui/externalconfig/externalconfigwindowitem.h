#pragma once

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "backend/backend.h"
#include "commongraphics/bubblebutton.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"

namespace ExternalConfigWindow {

class ExternalConfigWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit ExternalConfigWindowItem(QGraphicsObject *parent, PreferencesHelper *preferencesHelper);
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setClickable(bool isClickable);
    void setIcon(QString iconPath);
    void setButtonText(QString text);

signals:
    void buttonClick();
    void escapeClick();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onEscClicked();
    void onButtonClicked();

    void onBackgroundOpacityChange(const QVariant &value);
    void onTextOpacityChange(const QVariant &value);
    void onEscTextOpacityChange(const QVariant &value);
    void onLanguageChanged();

private:
    CommonGraphics::EscapeButton *escButton_;
    CommonGraphics::BubbleButton *okButton_;

    double curEscTextOpacity_;
    double curBackgroundOpacity_;
    double curTextOpacity_;

    QString curIconPath_;

    // constants:
    static constexpr int ICON_POS_Y = 80;

    static constexpr int TITLE_POS_Y = 136;
    static constexpr int DESCRIPTION_POS_Y = 172;
    static constexpr int DESCRIPTION_WIDTH_MIN = 218;

    static constexpr int CONNECT_BUTTON_POS_X = 122;
    static constexpr int CONNECT_BUTTON_POS_Y = 250;

    void updatePositions();
};

}
