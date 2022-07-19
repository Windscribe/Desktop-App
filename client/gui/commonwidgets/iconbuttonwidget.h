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

    int width();
    int height();
    void setImage(QString imagePath);

    void setUnhoverHoverOpacity(double unhoverOpacity, double hoverOpacity);
    void animateOpacityChange(double targetOpacity);

    void updateSize();

signals:
    void sizeChanged(int width, int height);
    void hoverEnter();
    void hoverLeave();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void onOpacityChanged(const QVariant &value);

private:
    QString imagePath_;

    double unhoverOpacity_;
    double hoverOpacity_;

    double curOpacity_;
    QVariantAnimation opacityAnimation_;

    int width_ ;
    int height_;

};

}

#endif // ICONBUTTONWIDGET_H
