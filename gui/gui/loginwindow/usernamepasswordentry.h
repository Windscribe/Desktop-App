#ifndef USERNAMEPASSWORDENTRY_H
#define USERNAMEPASSWORDENTRY_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include <QGraphicsProxyWidget>
#include "commongraphics/custommenulineedit.h"
#include "commongraphics/clickablegraphicsobject.h"
#include "blockableqlineedit.h"

namespace LoginWindow {


class UsernamePasswordEntry : public ClickableGraphicsObject
{
    Q_OBJECT
public:

    explicit UsernamePasswordEntry(QString descriptionText, bool password, ScalableGraphicsObject * parent = nullptr);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    bool isActive();
    void clearActiveState();

    QString getText();

    void setError(bool error);

    void setOpacityByFactor(double newOpacityFactor);

    virtual void setClickable(bool clickable);

    void setFocus();
    virtual void updateScaling();

public slots:
    void activate();

signals:
    void activated();
    void keyPressed(QKeyEvent *event);

protected:
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);

private slots:
    void onDescriptionPosYChanged(const QVariant &value);
    void onLinePosChanged(const QVariant &value);

private:
    QGraphicsProxyWidget *userEntryProxy_;
    CustomMenuLineEdit *userEntryLine_;

    int curDescriptionPosY_;
    QString descriptionText_;
    QVariantAnimation descriptionPosYAnimation_;

    int curLinePos_;
    QColor curLineActiveColor_;
    QVariantAnimation linePosAnimation_;

    int height_;
    int width_;

    bool active_;

    double curDescriptionOpacity_;
    double curLineEditOpacity_;

    double curBackgroundLineOpacity_;
    double curForgroundLineOpacity_;

    void drawBottomLine(QPainter *painter, int left, int right, int bottom, int whiteLinePos, QColor activeColor);

    const int DESCRIPTION_POS_CLICKED   = 0;
    const int DESCRIPTION_POS_UNCLICKED = 16;
    const int DESCRIPTION_TEXT_HEIGHT   = 16;
};

}

#endif // USERNAMEPASSWORDENTRY_H
