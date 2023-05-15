#ifndef UPGRADEWINDOWITEM_H
#define UPGRADEWINDOWITEM_H

#include "iupgradewindow.h"
#include "backend/preferences/preferences.h"
#include "commongraphics/bubblebutton.h"
#include "commongraphics/textbutton.h"

namespace UpgradeWindow {

class UpgradeWindowItem : public ScalableGraphicsObject, public IUpgradeWindow
{
    Q_OBJECT
    Q_INTERFACES(IUpgradeWindow)
public:
    explicit UpgradeWindowItem(Preferences *preferences, ScalableGraphicsObject *parent = nullptr);
    ~UpgradeWindowItem() {}

    QGraphicsObject *getGraphicsObject() override { return this; }
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;
    void setHeight(int height) override;

signals:
    void acceptClick() override;
    void cancelClick() override;

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

    static constexpr int TITLE_POS_Y = 51;

    static constexpr int DESCRIPTION_WIDTH_MIN = 230;
    static constexpr int DESCRIPTION_POS_Y = 102;

    static constexpr int ACCEPT_BUTTON_POS_Y = 180;
    static constexpr int CANCEL_BUTTON_POS_Y = 236;
};

}

#endif // UPGRADEWINDOWITEM_H
