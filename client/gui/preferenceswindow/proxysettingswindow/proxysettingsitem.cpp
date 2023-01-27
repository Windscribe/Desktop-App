#include "proxysettingsitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "utils/protoenumtostring.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

static bool IsProxyOptionAllowed(int option_number) {
    // Temporarily hide Auto-detect and SOCKS proxy options in the GUI.
    return option_number != ProtoTypes::PROXY_OPTION_AUTODETECT &&
           option_number != ProtoTypes::PROXY_OPTION_SOCKS;
}

ProxySettingsItem::ProxySettingsItem(ScalableGraphicsObject *parent) : BaseItem(parent, 93),
    isExpanded_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    comboBoxProxyType_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Proxy"), "", 50, Qt::transparent, 0, false);

    const QList< QPair<QString, int> > types = ProtoEnumToString::instance().getEnums(ProtoTypes::ProxyOption_descriptor());
    for (const auto &p : types)
    {
        if (IsProxyOptionAllowed(p.second))
            comboBoxProxyType_->addItem(p.first, p.second);
    }
    comboBoxProxyType_->setCurrentItem(types.begin()->second);
    connect(comboBoxProxyType_, SIGNAL(currentItemChanged(QVariant)), SLOT(onProxyTypeChanged(QVariant)));


    editBoxAddress_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Address"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Enter Address"), false);
    connect(editBoxAddress_, SIGNAL(textChanged(QString)), SLOT(onAddressChanged(QString)));

    editBoxPort_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Port"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Enter Port"), false);
    connect(editBoxPort_, SIGNAL(textChanged(QString)), SLOT(onPortChanged(QString)));

    editBoxUsername_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Username"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Enter Username"), false);
    connect(editBoxUsername_, SIGNAL(textChanged(QString)), SLOT(onUsernameChanged(QString)));

    editBoxPassword_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Password"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem","Enter Password"), true);
    editBoxPassword_->setMasked(true);
    connect(editBoxPassword_, SIGNAL(textChanged(QString)), SLOT(onPasswordChanged(QString)));

    dividerLine_ = new DividerLine(this, 276);

    expandEnimation_.setStartValue(COLLAPSED_HEIGHT);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT);
    expandEnimation_.setDuration(150);
    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));

    updatePositions();
}

void ProxySettingsItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt()*G_SCALE);
}

void ProxySettingsItem::onAddressChanged(const QString &text)
{
    if (QString::fromStdString(curProxySettings_.address()) != text)
    {
        curProxySettings_.set_address(text.toStdString());
        emit proxySettingsChanged(curProxySettings_);
    }
}

void ProxySettingsItem::onPortChanged(const QString &text)
{
    if (QString::number(curProxySettings_.port()) != text)
    {
        curProxySettings_.set_port(text.toInt());
        emit proxySettingsChanged(curProxySettings_);
    }
}

void ProxySettingsItem::onUsernameChanged(const QString &text)
{
    if (QString::fromStdString(curProxySettings_.username()) != text)
    {
        curProxySettings_.set_username(text.toStdString());
        emit proxySettingsChanged(curProxySettings_);
    }
}

void ProxySettingsItem::onPasswordChanged(const QString &text)
{
    if (QString::fromStdString(curProxySettings_.password()) != text)
    {
        curProxySettings_.set_password(text.toStdString());
        emit proxySettingsChanged(curProxySettings_);
    }
}

void ProxySettingsItem::onLanguageChanged()
{
    /*QVariant proxySelected = comboBoxProxyType_->currentItem();
    comboBoxProxyType_->clear();

    const QList<ProxyType> types = ProxyType::allAvailableTypes();
    for (const ProxyType &p : types)
    {
        comboBoxProxyType_->addItem(p.toString(), (int)p.type());
    }
    comboBoxProxyType_->setCurrentItem(proxySelected.toInt());*/
}

void ProxySettingsItem::hideOpenPopups()
{
    comboBoxProxyType_->hideMenu();
}

void ProxySettingsItem::updatePositions()
{
    editBoxAddress_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    editBoxPort_->setPos(0, COLLAPSED_HEIGHT*G_SCALE + editBoxAddress_->boundingRect().height());
    editBoxUsername_->setPos(0, COLLAPSED_HEIGHT*G_SCALE + editBoxAddress_->boundingRect().height() + editBoxPort_->boundingRect().height());
    editBoxPassword_->setPos(0, COLLAPSED_HEIGHT*G_SCALE + editBoxAddress_->boundingRect().height() + editBoxPort_->boundingRect().height() + editBoxUsername_->boundingRect().height());

    dividerLine_->setPos(24*G_SCALE, COLLAPSED_HEIGHT*G_SCALE - dividerLine_->boundingRect().height());
}

void ProxySettingsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    //painter->fillRect(boundingRect(), QBrush(QColor(0, 255, 0)));
    /*QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);*/
    painter->setPen(QColor(255, 255, 255));
    /*painter->drawText(boundingRect().adjusted(16, 1, 0, -2), Qt::AlignVCenter, title_);*/
    /*painter->setPen(QColor(255, 255, 255));

    //QRect bottomHalfRect(16, 45, boundingRect().width() - 40 - checkBoxButton_->boundingRect().width(), 40);*/
    QRectF bottomHalfRect = boundingRect().adjusted(16*G_SCALE, 44*G_SCALE, -70*G_SCALE, 0);
    //painter->fillRect(bottomHalfRect, QBrush(QColor(255, 0 , 0)));

    painter->setOpacity(0.5 * initOpacity);
    QFont *fontSmall = FontManager::instance().getFont(12, true);
    painter->setFont(*fontSmall);
    painter->drawText(bottomHalfRect, tr("If your network has a LAN proxy, configure it here."));
}

void ProxySettingsItem::setProxySettings(const ProtoTypes::ProxySettings &ps)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(curProxySettings_, ps))
    {
        curProxySettings_ = ps;
        int current_proxy_option = curProxySettings_.proxy_option();
        if (!IsProxyOptionAllowed(current_proxy_option))
            current_proxy_option = ProtoTypes::PROXY_OPTION_NONE;

        if (current_proxy_option == ProtoTypes::PROXY_OPTION_HTTP || current_proxy_option == ProtoTypes::PROXY_OPTION_SOCKS)
        {
            isExpanded_ = true;
            onExpandAnimationValueChanged(EXPANDED_HEIGHT);
        }
        else
        {
            isExpanded_ = false;
            onExpandAnimationValueChanged(COLLAPSED_HEIGHT);
        }
        comboBoxProxyType_->setCurrentItem(current_proxy_option);

        editBoxAddress_->setText(QString::fromStdString(curProxySettings_.address()));
        if (curProxySettings_.port() == 0)
        {
            editBoxPort_->setText("");
        }
        else
        {
            editBoxPort_->setText(QString::number(curProxySettings_.port()));
        }
        editBoxUsername_->setText(QString::fromStdString(curProxySettings_.username()));
        editBoxPassword_->setText(QString::fromStdString(curProxySettings_.password()));
    }
}

void ProxySettingsItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(isExpanded_? EXPANDED_HEIGHT*G_SCALE : COLLAPSED_HEIGHT*G_SCALE);
    updatePositions();
}

bool ProxySettingsItem::hasItemWithFocus()
{
    return editBoxAddress_->lineEditHasFocus()  ||
           editBoxPort_->lineEditHasFocus()     ||
           editBoxUsername_->lineEditHasFocus() ||
           editBoxPassword_->lineEditHasFocus();
}

void ProxySettingsItem::onProxyTypeChanged(QVariant v)
{
    int new_proxy_option = v.toInt();
    if (!IsProxyOptionAllowed(new_proxy_option))
        new_proxy_option = ProtoTypes::PROXY_OPTION_NONE;

    if ((new_proxy_option == ProtoTypes::PROXY_OPTION_HTTP || new_proxy_option == ProtoTypes::PROXY_OPTION_SOCKS)  && !isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Forward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = true;
    }
    else if ((new_proxy_option == ProtoTypes::PROXY_OPTION_NONE || new_proxy_option == ProtoTypes::PROXY_OPTION_AUTODETECT) && isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Backward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = false;
    }

    if ((int)curProxySettings_.proxy_option() != new_proxy_option)
    {
        curProxySettings_.set_proxy_option((ProtoTypes::ProxyOption)new_proxy_option);
        emit proxySettingsChanged(curProxySettings_);
    }
}


} // namespace PreferencesWindow
