#ifndef FOOTER_H
#define FOOTER_H

#include <QWidget>

namespace GuiLocations {

// TODO: expand to handle all footer responsibilities -- not just viewport fragment overlay
class Footer : public QWidget
{
    Q_OBJECT
public:
    explicit Footer(QWidget *parent = nullptr);

signals:

protected:
    void paintEvent(QPaintEvent *event);

private:
    // TODO: centralize
    static constexpr int FOOTER_HEIGHT = 2; // 20 + COVER_LAST_ITEM_LINE
    static constexpr int COVER_LAST_ITEM_LINE = 2;
};

}
#endif // FOOTER_H
