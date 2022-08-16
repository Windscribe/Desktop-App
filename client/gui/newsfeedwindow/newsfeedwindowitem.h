#ifndef NEWSFEEDWINDOWITEM_H
#define NEWSFEEDWINDOWITEM_H

#include <QTextBrowser>
#include "../backend/backend.h"
#include "commongraphics/scalablegraphicsobject.h"
#include "commongraphics/resizebar.h"
#include "commongraphics/escapebutton.h"
#include "commongraphics/iconbutton.h"
#include "commongraphics/scrollarea.h"
#include "newsfeedwindow/inewsfeedwindow.h"
#include "newsfeedwindow/newscontentitem.h"

namespace NewsFeedWindow {

class NewsFeedWindowItem : public ScalableGraphicsObject, public INewsFeedWindow
{
    Q_OBJECT
    Q_INTERFACES(INewsFeedWindow)
public:
    explicit NewsFeedWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QGraphicsObject *getGraphicsObject() override;

    int recommendedHeight() override;
    void setHeight(int height) override;

    void setScrollBarVisibility(bool on) override;

    QRectF boundingRect() const override;
    void setClickable(bool isClickable) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void updateScaling() override;

    void updateRead() override;

    virtual void setMessages(const QVector<types::Notification> &arr,
                             const QSet<qint64> &shownIds) override;
    virtual void setMessagesWithCurrentOverride(const QVector<types::Notification> &arr,
                                                const QSet<qint64> &shownIds,
                                                int overrideCurrentMessageId) override;

signals:
    void messageRead(qint64 messageId) override;
    void escClick() override;
    void sizeChanged() override;
    void resizeFinished() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onBackArrowButtonClicked();
    void onResizeStarted();
    void onResizeChange(int y);
    void onAppSkinChanged(APP_SKIN s);

private:
    static constexpr int BOTTOM_AREA_HEIGHT = 16;
    static constexpr int MIN_HEIGHT = 502;
    static constexpr int BOTTOM_RESIZE_ORIGIN_X = 155;
    static constexpr int BOTTOM_RESIZE_OFFSET_Y = 13;

    Preferences *preferences_;

    int curHeight_;
    double curScale_;
    int heightAtResizeStart_;
    QString backgroundBase_;
    QString backgroundHeader_;
    bool roundedFooter_;
    QColor footerColor_;

    IconButton *backArrowButton_;
    CommonGraphics::ResizeBar *bottomResizeItem_;
    CommonGraphics::EscapeButton *escapeButton_;
    CommonGraphics::ScrollArea *scrollAreaItem_;
    NewsContentItem *contentItem_;

    QRectF getBottomResizeArea();
    void updateChildItemsAfterHeightChanged();
    void updatePositions();
};

}

#endif // NEWSFEEDWINDOWITEM_H
