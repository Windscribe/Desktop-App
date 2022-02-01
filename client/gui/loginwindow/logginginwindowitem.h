#ifndef LOGGINGINWINDOWITEM_H
#define LOGGINGINWINDOWITEM_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "ilogginginwindow.h"
#include "loginbutton.h"

namespace LoginWindow {

class LoggingInWindowItem : public ScalableGraphicsObject, public ILoggingInWindow
{
    Q_OBJECT
    Q_INTERFACES(ILoggingInWindow)
public:

    explicit LoggingInWindowItem(QGraphicsObject *parent = nullptr);

    QGraphicsObject *getGraphicsObject() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void startAnimation() override;
    void stopAnimation() override;

    void setMessage(const QString &msg) override;
    void setAdditionalMessage(const QString &msg) override;

    void updateScaling() override;

private slots:
    void onSpinnerRotationChanged(const QVariant &value);

private:
    int curSpinnerRotation_;
    QVariantAnimation spinnerAnimation_;

    bool animationActive_;

    int targetRotation_;

    QString message_;
    QString additionalMessage_;

};

}

#endif // LOGGINGINWINDOWITEM_H
