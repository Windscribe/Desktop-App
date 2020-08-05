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

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    bool isActive();
    void clearActiveState();

    QString getText();

    void setError(bool error);

    void setOpacityByFactor(double newOpacityFactor);

    void setClickable(bool clickable) override;

    void setFocus();
    void updateScaling() override;

public slots:
    void activate();

signals:
    void activated();
    void keyPressed(QKeyEvent *event);

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;

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

    static constexpr int DESCRIPTION_POS_CLICKED   = 0;
    static constexpr int DESCRIPTION_POS_UNCLICKED = 16;
    static constexpr int DESCRIPTION_TEXT_HEIGHT   = 16;
};

}

#endif // USERNAMEPASSWORDENTRY_H
