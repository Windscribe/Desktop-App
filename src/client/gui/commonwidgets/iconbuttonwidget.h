#pragma once

#include <QPushButton>
#include <QVariant>
#include <QVariantAnimation>

#include "commongraphics/commongraphics.h"

namespace CommonWidgets {

class IconButtonWidget : public QPushButton
{
    Q_OBJECT
public:
    explicit IconButtonWidget(const QString &imagePath, const QString &text, QWidget * parent = nullptr);
    explicit IconButtonWidget(QString imagePath, QWidget * parent = nullptr);

    int width();
    int height();
    void setText(const QString &text);
    void setImage(const QString &imagePath);
    void setFont(const QFont &font);

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
    QString text_;
    QFont font_;

    double unhoverOpacity_ = OPACITY_UNHOVER_ICON_STANDALONE;
    double hoverOpacity_ = OPACITY_FULL;
    double curOpacity_ = OPACITY_UNHOVER_ICON_STANDALONE;
    QVariantAnimation opacityAnimation_;

    int width_ = 0;
    int height_ = 0;
};

}
