#pragma once

#include <QSharedPointer>
#include "commongraphics/baseitem.h"
#include "commongraphics/togglebutton.h"
#include "graphicresources/fontdescr.h"
#include "graphicresources/independentpixmap.h"
#include "api_responses/robertfilter.h"

namespace PreferencesWindow {

class RobertItem : public CommonGraphics::BaseItem
{
    Q_OBJECT
public:
    explicit RobertItem(ScalableGraphicsObject *parent, const api_responses::RobertFilter &filter);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setEnabled(bool enabled);
    void setState(bool isChecked);
    bool isChecked() const;

    void setLineVisible(bool visible);
    QPointF getButtonScenePos() const;

    void updateScaling() override;
    void setIcon(QSharedPointer<IndependentPixmap> icon);
    void setCaptionFont(const FontDescr &fontDescr);

signals:
    void filterChanged(const api_responses::RobertFilter &filter);
    void buttonHoverEnter();
    void buttonHoverLeave();

private slots:
    void onStateChanged(bool isChecked);
    void onLanguageChanged();

private:
    QSharedPointer<IndependentPixmap> iconForId(QString id);

    static constexpr int ROBERT_ITEM_HEIGHT = 58;
    static constexpr int ROBERT_ICON_MARGIN_Y = 21;
    static constexpr int ROBERT_CHECKBOX_MARGIN_Y = 18;
    static constexpr int ROBERT_TEXT_FIRST_MARGIN_Y = 13;
    static constexpr int ROBERT_TEXT_SECOND_MARGIN_Y = 30;
    static constexpr int ROBERT_TEXT_GAP_Y = 2;

    QString strCaption_;

    ToggleButton *checkBoxButton_;
    FontDescr captionFont_;
    QSharedPointer<IndependentPixmap> icon_;

    api_responses::RobertFilter filter_;
};

} // namespace PreferencesWindow
