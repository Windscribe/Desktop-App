#pragma once

#include <QGraphicsObject>
#include <QGraphicsSceneMouseEvent>
#include "commongraphics/baseitem.h"

namespace PreferencesWindow {

class TitleItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit TitleItem(ScalableGraphicsObject *parent, const QString &title = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setTitle(const QString &title);
    void setLink(const QString &linkText);

signals:
    void linkClicked();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private slots:
    void onColorAnimationValueChanged(const QVariant &value);

private:
    static constexpr int ACCOUNT_TITLE_HEIGHT = 14;
    QString title_;
    QString linkText_;
    QString linkUrl_;
    QColor linkColor_;
    QColor currentLinkColor_;

    QRectF linkRect_;

    double animProgress_ = 0;
    QVariantAnimation colorAnimation_;

    void hover();
    void unhover();
};

} // namespace PreferencesWindow
