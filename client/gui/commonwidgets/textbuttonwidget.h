#ifndef TEXTBUTTONWIDGET_H
#define TEXTBUTTONWIDGET_H

#include <QPushButton>
#include <QVariant>
#include <QVariantAnimation>
#include "graphicresources/fontdescr.h"

namespace CommonWidgets {

class TextButtonWidget : public QPushButton
{
    Q_OBJECT
public:
    explicit TextButtonWidget(QString text, QWidget * parent = nullptr);

    QSize sizeHint() const override;

    void setFont(const FontDescr &fontDescr);

    void setUnhoverHoverOpacity(double unhoverOpacity, double hoverOpacity);
    void animateOpacityChange(double targetOpacity);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void onOpacityChanged(const QVariant &value);
    void resetHoverState();

private:
    double unhoverOpacity_;
    double hoverOpacity_;

    double curOpacity_;
    QVariantAnimation opacityAnimation_;

    FontDescr fontDescr_;

    int width_  = 92;
    int height_ = 26;
    int arcsize_ = 10;
};

}

#endif // TEXTBUTTONWIDGET_H
