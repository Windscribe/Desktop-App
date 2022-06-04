#ifndef SCROLLABLEMENUBUTTON_H
#define SCROLLABLEMENUBUTTON_H

#include <QPushButton>
#include <QVariant>

class ComboMenuWidgetButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ComboMenuWidgetButton(const QVariant &d);

    QVariant data();

    void setWidthUnscaled(int width);
    int widthUnscaled();
    void setHeightUnscaled(int height);
    int heightUnscaled();

signals:
    void hoverEnter();

protected:
    virtual void enterEvent(QEnterEvent *e);

private:
    int widthUnscaled_;
    int heightUnscaled_;
    QVariant data_;
};

#endif // SCROLLABLEMENUBUTTON_H
