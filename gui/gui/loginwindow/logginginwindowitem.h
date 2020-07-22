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

    virtual QGraphicsObject *getGraphicsObject();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    virtual void startAnimation();
    virtual void stopAnimation();

    virtual void setMessage(const QString &msg);
    virtual void setAdditionalMessage(const QString &msg);

    virtual void updateScaling();

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
