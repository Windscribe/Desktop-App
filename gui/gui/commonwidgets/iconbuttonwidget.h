#ifndef ICONBUTTONWIDGET_H
#define ICONBUTTONWIDGET_H

#include <QPushButton>
#include <QVariant>
#include <QVariantAnimation>

namespace CommonWidgets {

class IconButtonWidget : public QPushButton
{
    Q_OBJECT
public:
    explicit IconButtonWidget(QString imagePath, QWidget * parent = nullptr);

    QSize sizeHint() const override;

    void setImage(QString imagePath);

    void setUnhoverHoverOpacity(double unhoverOpacity, double hoverOpacity);
    void animateOpacityChange(double targetOpacity);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void onOpacityChanged(const QVariant &value);

private:
    QString imagePath_;

    double unhoverOpacity_;
    double hoverOpacity_;

    double curOpacity_;
    QVariantAnimation opacityAnimation_;

    int width_  = 20;
    int height_ = 20;

};

}

#endif // ICONBUTTONWIDGET_H
