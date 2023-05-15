#ifndef TABBUTTON_H
#define TABBUTTON_H

#include <QString>
#include <QVariantAnimation>
#include "commongraphics/clickablegraphicsobject.h"
#include "graphicresources/independentpixmap.h"
#include "ipreferencestabcontrol.h"

namespace PreferencesWindow {

class TabButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    enum flags {
        TAB_BUTTON_FULL_OPACITY = 1 // Always full opacity, retain original color
    };

    explicit TabButton(ScalableGraphicsObject *parent, PREFERENCES_TAB_TYPE type, QString path, uint32_t flags = 0);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void updateScaling() override;

    void setSelected(bool selected) override;
    void setText(QString text);

    PREFERENCES_TAB_TYPE tab();

    static constexpr int BUTTON_WIDTH = 32;
    static constexpr int BUTTON_HEIGHT = 32;

signals:
    void tabClicked(PREFERENCES_TAB_TYPE tab, TabButton *source);

private slots:
    void onClicked();
    void onHoverEnter();
    void onHoverLeave();
    void onIconOpacityChanged(const QVariant &value);
    void onCircleOpacityChanged(const QVariant &value);

private:
    static constexpr double OPACITY_UNHOVER_CIRCLE = 0.1;
    static constexpr int ICON_MARGIN = 8;
    static constexpr int ICON_WIDTH = 16;
    static constexpr int ICON_HEIGHT = 16;
    static constexpr int ICON_SELECTED_COLOR_RED = 24;
    static constexpr int ICON_SELECTED_COLOR_GREEN = 34;
    static constexpr int ICON_SELECTED_COLOR_BLUE = 47;

    // Keep tooltip 2px to the right of the pic: 10 = 16 / 2 + 2.
    static constexpr int SCENE_OFFSET_X = BUTTON_WIDTH / 2 + 10;
    static constexpr int SCENE_OFFSET_Y = 7;

    void showTooltip();
    void hideTooltip();

    PREFERENCES_TAB_TYPE type_;
    QSharedPointer<IndependentPixmap> icon_;
    QString text_;

    QVariantAnimation iconOpacityAnimation_;
    QVariantAnimation circleOpacityAnimation_;
    double circleOpacity_;
    double iconOpacity_;
    uint32_t flags_;
};

} // namespace PreferencesWindow

#endif // TABBUTTON_H