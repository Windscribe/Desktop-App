#pragma once

#include <QSharedPointer>

#include "commongraphics/baseitem.h"
#include "commongraphics/togglebutton.h"
#include "commongraphics/iconbutton.h"
#include "graphicresources/fontdescr.h"
#include "graphicresources/independentpixmap.h"

namespace PreferencesWindow {

class ToggleItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit ToggleItem(ScalableGraphicsObject *parent, const QString &caption = "", const QString &tooltip = "");

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setEnabled(bool enabled);
    void setState(bool isChecked);
    bool isChecked() const;

    void setLineVisible(bool visible);
    QPointF getButtonScenePos() const;

    void updateScaling() override;
    void setIcon(QSharedPointer<IndependentPixmap> icon);
    void setCaption(const QString &caption);
    void setCaptionFont(const FontDescr &fontDescr);

    void setDescription(const QString &description, const QString &url = "");

signals:
    void stateChanged(bool isChecked);
    void buttonHoverEnter();
    void buttonHoverLeave();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onInfoIconClicked();

private:
    // Height of typical BaseItem (46) - height of checkbox item (22), divided by 2.
    static constexpr int CHECKBOX_MARGIN_Y = 12;

    QString strCaption_;
    QString strTooltip_;

    ToggleButton *checkBoxButton_;
    FontDescr captionFont_;
    QSharedPointer<IndependentPixmap> icon_;
    bool isCaptionElided_ = false;
    QRectF captionRect_;

    QString desc_;
    QString descUrl_;
    IconButton *infoIcon_;
    int descHeight_ = 0;
    int descRightMargin_ = 0;

    void updatePositions();
};

} // namespace PreferencesWindow
