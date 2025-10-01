#pragma once
#include <QWidget>
#include "commonwidgets/textbuttonwidget.h"

namespace GuiLocations {

class EmptyListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EmptyListWidget(QWidget *parent);

    void setText(const QString &text, int width);
    void setIcon(const QString &icon);
    void setButton(const QString &buttonText);

    void updateScaling();

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event);

private:
    QString icon_;
    QString text_;
    int textWidth_;
    int textHeight_;
    CommonWidgets::TextButtonWidget *button_;

    void updateButtonPos();
};

} // namespace GuiLocation
