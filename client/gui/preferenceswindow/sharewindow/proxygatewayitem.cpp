#include "proxygatewayitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "utils/protoenumtostring.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

ProxyGatewayItem::ProxyGatewayItem(ScalableGraphicsObject *parent, PreferencesHelper *preferencesHelper) : BaseItem(parent, 87)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    checkBoxButton_ = new CheckBoxButton(this);
    connect(checkBoxButton_, SIGNAL(stateChanged(bool)), SLOT(onCheckBoxStateChanged(bool)));

    //descriptionFont_ =  *FontManager::instance().getFont(12, false);
    updateCollapsedAndExpandedHeights();

    line_ = new DividerLine(this, 276);

    comboBoxProxyType_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Proxy Type"), "", 43, QColor(16, 22, 40), 24, true);

    const QList< QPair<QString, int> > allProxyTypes = ProtoEnumToString::instance().getEnums(ProtoTypes::ProxySharingMode_descriptor());
    for (const auto &p : allProxyTypes)
    {
        comboBoxProxyType_->addItem(p.first, p.second);
    }
    comboBoxProxyType_->setCurrentItem(allProxyTypes.begin()->second);
    connect(comboBoxProxyType_, SIGNAL(currentItemChanged(QVariant)), SLOT(onProxyTypeItemChanged(QVariant)));

    proxyIpAddressItem_ = new ProxyIpAddressItem(this, true);
    connect(preferencesHelper, SIGNAL(proxyGatewayAddressChanged(QString)), SLOT(onProxyGatewayAddressChanged(QString)));

    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));
    expandEnimation_.setStartValue(collapsedHeight_);
    expandEnimation_.setEndValue(expandedHeight_);
    expandEnimation_.setDuration(150);

    updatePositions();
}

void ProxyGatewayItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    QRect topHalfRect(0, 0, boundingRect().width(), 50*G_SCALE);
    QFont *font = FontManager::instance().getFont(13, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));

    painter->drawText(topHalfRect.adjusted(16*G_SCALE, 0, 0, 0), Qt::AlignVCenter, tr("Proxy Gateway"));

    // desc text
    painter->setOpacity(0.5 * initialOpacity);
    QFont *descriptionFont =  FontManager::instance().getFont(12, false);
    painter->setFont(*descriptionFont);
    painter->drawText(descriptionRect_, tr(descriptionText_.toStdString().c_str()));

}

void ProxyGatewayItem::setProxyGatewayPars(const ProtoTypes::ShareProxyGateway &sp)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(sp_, sp))
    {
        sp_ = sp;
        if (sp.is_enabled())
        {
            checkBoxButton_->setState(true);
            setHeight(expandedHeight_);
        }
        else
        {
            checkBoxButton_->setState(false);
            setHeight(collapsedHeight_);
        }

        comboBoxProxyType_->setCurrentItem((int)sp.proxy_sharing_mode());
    }
}

void ProxyGatewayItem::updateScaling()
{
    BaseItem::updateScaling();
    updateCollapsedAndExpandedHeights();
    expandEnimation_.setStartValue(collapsedHeight_);
    expandEnimation_.setEndValue(expandedHeight_);
    setHeight(checkBoxButton_->isChecked() ? expandedHeight_ : collapsedHeight_);
    updatePositions();
}

void ProxyGatewayItem::updateCollapsedAndExpandedHeights()
{
    QFont *descriptionFont =  FontManager::instance().getFont(12, false);
    descriptionText_ = QT_TR_NOOP("Configure your TV, gaming console or other devices that support proxy servers");
    descriptionRect_ = CommonGraphics::textBoundingRect(*descriptionFont, 16*G_SCALE, 45*G_SCALE,
                                                            boundingRect().width() - 40*G_SCALE - checkBoxButton_->boundingRect().width(),
                                                            tr(descriptionText_.toStdString().c_str()), Qt::TextWordWrap);
    collapsedHeight_ = descriptionRect_.y() + descriptionRect_.height() + 10*G_SCALE;
    expandedHeight_ = collapsedHeight_ + (43 + 43)*G_SCALE;
}

void ProxyGatewayItem::updatePositions()
{
    QRect topHalfRect(0, 0, boundingRect().width(), 50*G_SCALE);
    checkBoxButton_->setPos(topHalfRect.width() - checkBoxButton_->boundingRect().width() - 16*G_SCALE, (topHalfRect.height() - checkBoxButton_->boundingRect().height()) / 2);

    line_->setPos(24*G_SCALE, collapsedHeight_ - 3*G_SCALE);
    comboBoxProxyType_->setPos(0, collapsedHeight_);
    proxyIpAddressItem_->setPos(0, collapsedHeight_ + comboBoxProxyType_->boundingRect().height());
}

void ProxyGatewayItem::onCheckBoxStateChanged(bool isChecked)
{
    if (isChecked)
    {
        expandEnimation_.setDirection(QVariantAnimation::Forward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
    }
    else
    {
        expandEnimation_.setDirection(QVariantAnimation::Backward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
    }

    sp_.set_is_enabled(isChecked);
    emit proxyGatewayParsChanged(sp_);
}

void ProxyGatewayItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt());
}

void ProxyGatewayItem::onProxyTypeItemChanged(QVariant v)
{
    sp_.set_proxy_sharing_mode((ProtoTypes::ProxySharingMode)v.toInt());
    emit proxyGatewayParsChanged(sp_);
}

void ProxyGatewayItem::onProxyGatewayAddressChanged(const QString &address)
{
    proxyIpAddressItem_->setIP(address);
}

void ProxyGatewayItem::onLanguageChanged()
{
    updateCollapsedAndExpandedHeights();
    updatePositions();
}

void ProxyGatewayItem::hideOpenPopups()
{
    comboBoxProxyType_->hideMenu();
}

} // namespace PreferencesWindow
