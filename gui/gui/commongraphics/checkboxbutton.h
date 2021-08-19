#ifndef CHECKBOXBUTTON_H
#define CHECKBOXBUTTON_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "clickablegraphicsobject.h"

namespace PreferencesWindow {

class CheckBoxButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit CheckBoxButton(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setState(bool isChecked);
    bool isChecked() const;

signals:
    void stateChanged(bool isChecked);

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onOpacityChanged(const QVariant &value);

private:
    qreal curOpacity_; // 0 - unchecked(white background), 1 - checked(green background)
    bool isChecked_;
    QVariantAnimation opacityAnimation_;

};

} // namespace PreferencesWindow

#endif // CHECKBOXBUTTON_H
