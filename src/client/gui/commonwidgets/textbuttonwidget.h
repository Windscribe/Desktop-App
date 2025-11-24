#pragma once

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

    void setText(const QString &text);
    void setFont(const FontDescr &fontDescr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void updateWidth();

    FontDescr fontDescr_;

    int width_  = 92;
    int height_ = 34;
    int arcsize_ = 10;
};

}
