#pragma once

#include "backend/preferences/preferences.h"
#include "commongraphics/bubblebutton.h"
#include "commongraphics/textbutton.h"

namespace UpgradeWindow {

class UpgradeWindowItem : public ScalableGraphicsObject
{
    Q_OBJECT
public:
    explicit UpgradeWindowItem(Preferences *preferences, ScalableGraphicsObject *parent = nullptr);
    ~UpgradeWindowItem() {}
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setHeight(int height);

signals:
    void acceptClick();
    void cancelClick();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onLanguageChanged();
    void updatePositions();
    void onAppSkinChanged(APP_SKIN s);

private:
    Preferences *preferences_;

    CommonGraphics::BubbleButton *acceptButton_;
    CommonGraphics::TextButton *cancelButton_;

    int height_;

    static constexpr int TITLE_POS_Y = 35;

    static constexpr int DESCRIPTION_WIDTH_MIN = 230;
    static constexpr int DESCRIPTION_POS_Y = 86;

    static constexpr int ACCEPT_BUTTON_POS_Y = 164;
    static constexpr int CANCEL_BUTTON_POS_Y = 212;
};

}
