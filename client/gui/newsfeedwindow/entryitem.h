#ifndef ENTRYITEM_H
#define ENTRYITEM_H

#include <QTextBrowser>
#include <QVariantAnimation>

#include "../backend/backend.h"
#include "commongraphics/baseitem.h"
#include "graphicresources/independentpixmap.h"
#include "messageitem.h"

namespace NewsFeedWindow {

class EntryItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    EntryItem(ScalableGraphicsObject *parent, const types::Notification &item, int width);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void updateScaling() override;

    bool expanded();
    int collapsedHeight();
    int expandedHeight();
    void setExpanded(bool expanded, bool read = true);

    void setAccented(bool accented);
    void setRead(bool read);

signals:
    void messageRead(qint64 id);
    void scrollToItem(EntryItem *item);

private slots:
    void onExpandRotationAnimationValueChanged(const QVariant &value);
    void onOpacityAnimationValueChanged(const QVariant &value);
    void onClicked();
    void onHoverEnter();
    void onHoverLeave();

private:
    static constexpr int COLLAPSED_HEIGHT = 48;
    static constexpr int ROUNDED_RECT_RADIUS = 16;
    static constexpr int TEXT_MARGIN = 16;
    static constexpr int READ_MARKER_WIDTH = 8;
    static constexpr int DATE_WIDTH = 88;
    static constexpr int ICON_HEIGHT = 16;
    static constexpr int ICON_WIDTH = 16;

    types::Notification item_;
    bool expanded_;
    bool accented_;
    double expandAnimationProgress_;
    double opacityAnimationProgress_;
    double textOpacity_;
    double plusIconOpacity_;
    QVariantAnimation expandAnimation_;
    QVariantAnimation opacityAnimation_;
    bool read_;
    int titleHeight_;
    int expandedHeight_;
    MessageItem *messageItem_;
    QSharedPointer<IndependentPixmap> icon_;

    void updatePositions();
};

}
#endif // ENTRYITEM_H
