#include "robertitem.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"

namespace PreferencesWindow {

RobertItem::RobertItem(ScalableGraphicsObject *parent, const api_responses::RobertFilter &filter)
  : BaseItem(parent, ROBERT_ITEM_HEIGHT*G_SCALE), captionFont_(14, QFont::Bold), icon_(nullptr), filter_(filter)
{
    checkBoxButton_ = new ToggleButton(this);
    connect(checkBoxButton_, &ToggleButton::stateChanged, this, &RobertItem::onStateChanged);
    connect(checkBoxButton_, &ToggleButton::hoverEnter, this, &RobertItem::buttonHoverEnter);
    connect(checkBoxButton_, &ToggleButton::hoverLeave, this, &RobertItem::buttonHoverLeave);

    strCaption_ = tr(filter.title.toStdString().c_str());
    setIcon(iconForId(filter.id));
    setState(filter.status == 1);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &RobertItem::onLanguageChanged);
    onLanguageChanged();

    updateScaling();
}

void RobertItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font = FontManager::instance().getFont(captionFont_);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_FULL);

    int xOffset = PREFERENCES_MARGIN_X*G_SCALE;
    if (icon_) {
        xOffset = (PREFERENCES_MARGIN_X + 8 + ICON_WIDTH)*G_SCALE;
        icon_->draw(PREFERENCES_MARGIN_X*G_SCALE, ROBERT_ICON_MARGIN_Y*G_SCALE, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter);
    }

    // Title text
    painter->drawText(boundingRect().adjusted(xOffset,
                                              ROBERT_TEXT_FIRST_MARGIN_Y*G_SCALE,
                                              -PREFERENCES_MARGIN_X*G_SCALE,
                                             -(ROBERT_TEXT_GAP_Y + ROBERT_TEXT_FIRST_MARGIN_Y)*G_SCALE),
                      Qt::AlignLeft,
                      strCaption_);

    // Bottom text (Allowing or Blocking)
    painter->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    QString text = tr("Blocking");

    if (checkBoxButton_->isChecked()) {
        painter->setOpacity(OPACITY_FULL);
        painter->setPen(FontManager::instance().getSeaGreenColor());
    } else {
        text = tr("Allowing");
        painter->setOpacity(OPACITY_SIXTY);
        painter->setPen(Qt::white);
    }
    painter->drawText(boundingRect().adjusted(xOffset,
                                              ROBERT_TEXT_SECOND_MARGIN_Y*G_SCALE,
                                              -PREFERENCES_MARGIN_X*G_SCALE,
                                              0),
                        Qt::AlignLeft,
                        text);
}

void RobertItem::setState(bool isChecked)
{
    checkBoxButton_->setState(isChecked);
    update();
}

bool RobertItem::isChecked() const
{
    return checkBoxButton_->isChecked();
}

QPointF RobertItem::getButtonScenePos() const
{
    return checkBoxButton_->scenePos();
}

void RobertItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(ROBERT_ITEM_HEIGHT*G_SCALE);
    checkBoxButton_->setPos(boundingRect().width() - checkBoxButton_->boundingRect().width() - PREFERENCES_MARGIN_X*G_SCALE,
                            ROBERT_CHECKBOX_MARGIN_Y*G_SCALE);
}

void RobertItem::setIcon(QSharedPointer<IndependentPixmap> icon)
{
    icon_ = icon;
}

void RobertItem::setCaptionFont(const FontDescr &fontDescr)
{
    captionFont_ = fontDescr;
}

void RobertItem::onStateChanged(bool checked)
{
    update();
    filter_.status = checked;
    emit filterChanged(filter_);
}

QSharedPointer<IndependentPixmap> RobertItem::iconForId(QString id)
{
    if (id == "malware") {
        return ImageResourcesSvg::instance().getIndependentPixmap("preferences/MALWARE");
    } else if (id == "ads") {
        return ImageResourcesSvg::instance().getIndependentPixmap("preferences/ADS");
    } else if (id == "social") {
        return ImageResourcesSvg::instance().getIndependentPixmap("preferences/SOCIAL_NETWORKS");
    } else if (id == "porn") {
        return ImageResourcesSvg::instance().getIndependentPixmap("preferences/PORN");
    } else if (id == "gambling") {
        return ImageResourcesSvg::instance().getIndependentPixmap("preferences/GAMBLING");
    } else if (id == "fakenews") {
        return ImageResourcesSvg::instance().getIndependentPixmap("preferences/FAKE_NEWS");
    } else if (id == "competitors") {
        return ImageResourcesSvg::instance().getIndependentPixmap("preferences/OTHER_VPNS");
    } else if (id == "cryptominers") {
        return ImageResourcesSvg::instance().getIndependentPixmap("preferences/CRYPTO");
    } else {
        return ImageResourcesSvg::instance().getIndependentPixmap("preferences/UNKNOWN_FILTER");
    }
}

void RobertItem::onLanguageChanged()
{
    update();
}

} // namespace PreferencesWindow
