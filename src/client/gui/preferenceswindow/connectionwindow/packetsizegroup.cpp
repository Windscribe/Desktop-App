#include "packetsizegroup.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "packetsizeeditboxitem.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "tooltips/tooltiputil.h"
#include "tooltips/tooltipcontroller.h"

namespace PreferencesWindow {

PacketSizeGroup::PacketSizeGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    packetSizeModeItem_ = new ComboBoxItem(this);
    connect(packetSizeModeItem_, &ComboBoxItem::currentItemChanged, this, &PacketSizeGroup::onAutomaticChanged);
    packetSizeModeItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/PACKET_SIZE"));
    addItem(packetSizeModeItem_);

    editBoxItem_ = new PacketSizeEditBoxItem(this);
    connect(editBoxItem_, &PacketSizeEditBoxItem::textChanged, this, &PacketSizeGroup::onEditBoxTextChanged);
    connect(editBoxItem_, &PacketSizeEditBoxItem::detectButtonClicked, this, &PacketSizeGroup::detectPacketSize);
    connect(editBoxItem_, &PacketSizeEditBoxItem::detectButtonHoverEnter, this, &PacketSizeGroup::onDetectHoverEnter);
    connect(editBoxItem_, &PacketSizeEditBoxItem::detectButtonHoverLeave, this, &PacketSizeGroup::onDetectHoverLeave);
    addItem(editBoxItem_);

    QRegularExpression ipRegex ("^\\d+$");
    QRegularExpressionValidator *integerValidator = new QRegularExpressionValidator(ipRegex, this);
    editBoxItem_->setValidator(integerValidator);

    hideItems(indexOf(editBoxItem_), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &PacketSizeGroup::onLanguageChanged);
    onLanguageChanged();
}

void PacketSizeGroup::setPacketSizeSettings(types::PacketSize settings)
{
    if (settings != settings_)
    {
        settings_ = settings;
        packetSizeModeItem_->setCurrentItem(settings.isAutomatic ? 0 : 1);
        editBoxItem_->setText(QString::number(settings_.mtu));
        updateMode();
    }
}

void PacketSizeGroup::setPacketSizeDetectionState(bool on)
{
    editBoxItem_->setDetectButtonBusyState(on);
}

void PacketSizeGroup::onDetectHoverEnter()
{
    if (isShowingError_)
        return;

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(editBoxItem_->scenePos()));

    int posX = globalPt.x() + 194 * G_SCALE;
    int posY = globalPt.y() + 8 * G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE);
    ti.x = posX;
    ti.y = posY;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.8;
    ti.title = tr("Auto-Detect & Generate MTU");
    if (scene() && !scene()->views().isEmpty()) {
        ti.parent = scene()->views().first()->viewport();
    }
    TooltipController::instance().showTooltipBasic(ti);
}

void PacketSizeGroup::onDetectHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE);
    if (isShowingError_) {
        TooltipController::instance().hideTooltip(TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE_ERROR);
        isShowingError_ = false;
    }
}

void PacketSizeGroup::showPacketSizeDetectionError(const QString &title, const QString &message)
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE);

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(editBoxItem_->scenePos()));

    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_AUTO_DETECT_PACKET_SIZE_ERROR);
    ti.x = globalPt.x() + 194 * G_SCALE;
    ti.y = globalPt.y() + 8 * G_SCALE;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.8;
    ti.title = title;
    ti.desc = message;
    ti.width = 150 * G_SCALE;
    ti.delay = 100;
    if (scene() && !scene()->views().isEmpty()) {
        ti.parent = scene()->views().first()->viewport();
    }

    QTimer::singleShot(0, [this, ti]() {
        isShowingError_ = true;
        editBoxItem_->setDetectButtonSelectedState(true);
        TooltipController::instance().showTooltipBasic(ti);
    });

}

void PacketSizeGroup::onAutomaticChanged(QVariant value)
{
    settings_.isAutomatic = (value.toInt() == 0);
    updateMode();
    emit packetSizeChanged(settings_);
}

void PacketSizeGroup::updateMode()
{
    if (settings_.isAutomatic) {
        hideItems(indexOf(editBoxItem_));
    } else {
        showItems(indexOf(editBoxItem_));
    }
}

void PacketSizeGroup::onEditBoxTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        settings_.mtu = -1;
        editBoxItem_->setText(QString::number(settings_.mtu));
    } else {
        if (text.toInt() < 68 || text.toInt() > 65535) {
            editBoxItem_->setText(QString::number(settings_.mtu));
            return;
        }
        settings_.mtu = text.toInt();
    }
    emit packetSizeChanged(settings_);
}

void PacketSizeGroup::onLanguageChanged()
{
    packetSizeModeItem_->setLabelCaption(tr("Packet Size"));
    QList<QPair<QString, QVariant>> list;
    list << qMakePair(tr("Auto"), 0);
    list << qMakePair(tr("Manual"), 1);
    packetSizeModeItem_->setItems(list, settings_.isAutomatic ? 0 : 1);

    editBoxItem_->setCaption(tr("MTU"));
    editBoxItem_->setPrompt(tr("MTU"));
}

void PacketSizeGroup::setDescription(const QString &desc, const QString &descUrl)
{
    packetSizeModeItem_->setDescription(desc, descUrl);
}

} // namespace PreferencesWindow
