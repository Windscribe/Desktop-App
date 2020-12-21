#include "securehotspotitem.h"

#include <QPainter>
#include <QMessageBox>
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

extern QWidget *g_mainWindow;

namespace PreferencesWindow {

SecureHotspotItem::SecureHotspotItem(ScalableGraphicsObject *parent) : BaseItem(parent, 87), supported_(HOTSPOT_SUPPORTED)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    checkBoxButton_ = new CheckBoxButton(this);
    connect(checkBoxButton_, SIGNAL(stateChanged(bool)), SLOT(onCheckBoxStateChanged(bool)));

    setSupported(supported_);

    line_ = new DividerLine(this, 276);

    editBoxSSID_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "SSID"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter SSID"), false);
    connect(editBoxSSID_, SIGNAL(textChanged(QString)), SLOT(onSSIDChanged(QString)));

    editBoxPassword_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Password"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter Password"), true);
    connect(editBoxPassword_, SIGNAL(textChanged(QString)), SLOT(onPasswordChanged(QString))); 

    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));
    expandEnimation_.setStartValue(collapsedHeight_);
    expandEnimation_.setEndValue(expandedHeight_);
    expandEnimation_.setDuration(150);

    updatePositions();
}

void SecureHotspotItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    QRect topHalfRect(0, 0, boundingRect().width(), 50*G_SCALE);
    QFont *font = FontManager::instance().getFont(13, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));

    painter->drawText(topHalfRect.adjusted(16*G_SCALE, 0, 0, 0), Qt::AlignVCenter, tr("Secure Hotspot"));

    //QRect bottomHalfRect(16, 45, boundingRect().width() - 50 - checkBoxButton_->boundingRect().width(), 40);
    //painter->fillRect(bottomHalfRect, QBrush(QColor(255, 0 , 0)));

    painter->setOpacity(0.5 * initialOpacity);
    QFont *descriptionFont =  FontManager::instance().getFont(12, false);
    painter->setFont(*descriptionFont);
    painter->drawText(descriptionRect_, tr(descriptionText_.toStdString().c_str()));

}

void SecureHotspotItem::setSecureHotspotPars(const ProtoTypes::ShareSecureHotspot &ss)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(ss_, ss))
    {
        ss_ = ss;
        if (ss.is_enabled())
        {
            checkBoxButton_->setState(true);
            setHeight(expandedHeight_);
        }
        else
        {
            checkBoxButton_->setState(false);
            setHeight(collapsedHeight_);
        }

        editBoxSSID_->setText(QString::fromStdString(ss.ssid()));
        editBoxPassword_->setText(QString::fromStdString(ss.password()));
    }
}

void SecureHotspotItem::setSupported(HOTSPOT_SUPPORT_TYPE supported)
{
    supported_ = supported;
    checkBoxButton_->setEnabled(supported_ == HOTSPOT_SUPPORTED);
    if (supported_ != HOTSPOT_SUPPORTED)
    {
        checkBoxButton_->setState(false);
        ss_.set_is_enabled(false);
        emit secureHotspotParsChanged(ss_);
    }
    updateCollapsedAndExpandedHeight();
    setHeight(collapsedHeight_);
    update();
}

void SecureHotspotItem::updateScaling()
{
    BaseItem::updateScaling();
    updateCollapsedAndExpandedHeight();
    setHeight(checkBoxButton_->isChecked() ? expandedHeight_ : collapsedHeight_);
    updatePositions();
}

void SecureHotspotItem::onCheckBoxStateChanged(bool isChecked)
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

    ss_.set_is_enabled(isChecked);
    emit secureHotspotParsChanged(ss_);
}

void SecureHotspotItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt());
}

void SecureHotspotItem::onSSIDChanged(const QString &text)
{
    ss_.set_ssid(text.toStdString());
    emit secureHotspotParsChanged(ss_);
}

void SecureHotspotItem::onPasswordChanged(const QString &password)
{
    if (password.length() >= 8)
    {
        ss_.set_password(password.toStdString());
        emit secureHotspotParsChanged(ss_);
    }
    else
    {
        QString title = tr("Windscribe");
        QString desc = tr("Hotspot password must be at least 8 characters.");
        QMessageBox::information(g_mainWindow, title, desc);
        editBoxPassword_->setText(QString::fromStdString(ss_.password()));
    }
}

void SecureHotspotItem::onLanguageChanged()
{
    updateCollapsedAndExpandedHeight();
    updatePositions();
}

void SecureHotspotItem::updateCollapsedAndExpandedHeight()
{
    QFont *descriptionFont =  FontManager::instance().getFont(12, false);
    if (supported_ == HOTSPOT_SUPPORTED)
    {
        descriptionText_ = QT_TR_NOOP("Create a secure hotspot and allow others to use your secure connection");
    }
    else if (supported_ == HOTSPOT_NOT_SUPPORTED)
    {
        descriptionText_ = QT_TR_NOOP("Secure hotspot is not supported by your network adapter");
    }
    else if (supported_ == HOTSPOT_NOT_SUPPORTED_BY_IKEV2)
    {
        descriptionText_ = QT_TR_NOOP("Secure hotspot is not supported for IKEv2 protocol and automatic connection mode");
    }
    else
    {
        Q_ASSERT(false);
    }

    descriptionRect_ = CommonGraphics::textBoundingRect(*descriptionFont, 16*G_SCALE, 45*G_SCALE,
                                                            boundingRect().width() - 40*G_SCALE - checkBoxButton_->boundingRect().width(),
                                                            tr(descriptionText_.toStdString().c_str()), Qt::TextWordWrap);
    collapsedHeight_ = descriptionRect_.y() + descriptionRect_.height() + 10*G_SCALE;
    expandedHeight_ = collapsedHeight_ + (43 + 43)*G_SCALE;

    expandEnimation_.setStartValue(collapsedHeight_);
    expandEnimation_.setEndValue(expandedHeight_);
}

void SecureHotspotItem::updatePositions()
{
    QRect topHalfRect(0, 0, boundingRect().width(), 50*G_SCALE);
    checkBoxButton_->setPos(topHalfRect.width() - checkBoxButton_->boundingRect().width() - 16*G_SCALE, (topHalfRect.height() - checkBoxButton_->boundingRect().height()) / 2);
    line_->setPos(24*G_SCALE, collapsedHeight_ - 3*G_SCALE);
    editBoxSSID_->setPos(0, collapsedHeight_);
    editBoxPassword_->setPos(0, collapsedHeight_ + editBoxSSID_->boundingRect().height());
}

} // namespace PreferencesWindow
