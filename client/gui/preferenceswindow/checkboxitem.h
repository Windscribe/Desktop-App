#ifndef CHECKBOXITEM_H
#define CHECKBOXITEM_H

#include <QSharedPointer>
#include "commongraphics/baseitem.h"
#include "commongraphics/checkboxbutton.h"
#include "graphicresources/fontdescr.h"
#include "graphicresources/independentpixmap.h"

namespace PreferencesWindow {

class CheckBoxItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit CheckBoxItem(ScalableGraphicsObject *parent, const QString &caption = "", const QString &tooltip = "");

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

signals:
    void stateChanged(bool isChecked);
    void buttonHoverEnter();
    void buttonHoverLeave();

private:
    // Height of typical BaseItem (48) - height of checkbox item (22), divided by 2.
    static constexpr int CHECKBOX_MARGIN_Y = 13;

    QString strCaption_;
    QString strTooltip_;

    CheckBoxButton *checkBoxButton_;
    FontDescr captionFont_;
    QSharedPointer<IndependentPixmap> icon_;
};

} // namespace PreferencesWindow

#endif // CHECKBOXITEM_H
