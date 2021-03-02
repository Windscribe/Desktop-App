#ifndef FOOTER_H
#define FOOTER_H

#include <QWidget>

namespace GuiLocations {

class FooterTopStrip : public QWidget
{
    Q_OBJECT
public:
    explicit FooterTopStrip(QWidget *parent = nullptr);

    static constexpr int HEIGHT = 2;

protected:
    void paintEvent(QPaintEvent *event);
};

}
#endif // FOOTER_H
