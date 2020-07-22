#ifndef LOCATIONSTRAYMENUBUTTON_H
#define LOCATIONSTRAYMENUBUTTON_H

#include <QPushButton>

class LocationsTrayMenuButton : public QPushButton
{
    Q_OBJECT
public:
    explicit LocationsTrayMenuButton(QWidget *parent = 0);

    enum {TYPE_UP, TYPE_DOWN};
    void setType(int type);

protected:
    virtual void paintEvent(QPaintEvent *pEvent);

private:
    int type_;
};

#endif // LOCATIONSTRAYMENUBUTTON_H
