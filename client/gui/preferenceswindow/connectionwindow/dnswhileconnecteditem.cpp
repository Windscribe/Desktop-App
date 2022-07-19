#include "dnswhileconnecteditem.h"

#include "dpiscalemanager.h"

namespace PreferencesWindow {

DnsWhileConnectedItem::DnsWhileConnectedItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50),
   comboBoxDNS_(nullptr)
   ,isExpanded_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    comboBoxDNS_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "DNS While Connected"), "", 50, Qt::transparent, 0, false);
    const QList<DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE> modes = DnsWhileConnectedInfo::allAvailableTypes();
    for (const DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE &d : modes)
    {
        comboBoxDNS_->addItem(DnsWhileConnectedInfo::typeToString(d), static_cast<int>(d));
    }
    comboBoxDNS_->setCurrentItem(modes.first());
    connect(comboBoxDNS_, SIGNAL(currentItemChanged(QVariant)), SLOT(onDNSWhileConnectedModeChanged(QVariant)));

    editBoxIP_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "IP Address"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter IP"), false);
    connect(editBoxIP_, SIGNAL(textChanged(QString)), SLOT(onDNSWhileConnectedIPChanged(QString)));
    editBoxIP_->setPos(0, COLLAPSED_HEIGHT);

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegularExpression ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
    editBoxIP_->setValidator(ipValidator);

    dividerLine_ = new DividerLine(this, 276);
    setHeightAndLinePos(COLLAPSED_HEIGHT);

    expandEnimation_.setStartValue(COLLAPSED_HEIGHT);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT);
    expandEnimation_.setDuration(150);
    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));
}

void DnsWhileConnectedItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void DnsWhileConnectedItem::updateScaling()
{
    BaseItem::updateScaling();
    int height = isExpanded_ ? EXPANDED_HEIGHT : COLLAPSED_HEIGHT;
    editBoxIP_->setPos(0, COLLAPSED_HEIGHT*G_SCALE);
    setHeightAndLinePos(height);
    comboBoxDNS_->updateScaling();
}

bool DnsWhileConnectedItem::hasItemWithFocus()
{
    return editBoxIP_->lineEditHasFocus();
}

void DnsWhileConnectedItem::setDNSWhileConnected(const DnsWhileConnectedInfo &dns)
{
    if (dns != curDNSWhileConnected_)
    {
        curDNSWhileConnected_ = dns;

        // update inner widgets
        if (dns.type() == DnsWhileConnectedInfo::ROBERT)
        {
            comboBoxDNS_->setCurrentItem(DnsWhileConnectedInfo::ROBERT);
        }
        else
        {
            editBoxIP_->setText(dns.ipAddress());
            comboBoxDNS_->setCurrentItem(DnsWhileConnectedInfo::CUSTOM);
        }
        updateHeight(curDNSWhileConnected_.type());
    }
}

void PreferencesWindow::DnsWhileConnectedItem::updateHeight(DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE type)
{
    if (type == DnsWhileConnectedInfo::CUSTOM && !isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Forward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = true;
    }
    else if (isExpanded_)
    {
        expandEnimation_.setDirection(QVariantAnimation::Backward);
        if (expandEnimation_.state() != QVariantAnimation::Running)
        {
            expandEnimation_.start();
        }
        isExpanded_ = false;
    }
}

void DnsWhileConnectedItem::setHeightAndLinePos(int height)
{
    setHeight(height*G_SCALE);
    dividerLine_->setPos(24*G_SCALE, height*G_SCALE - dividerLine_->boundingRect().height());
}

void DnsWhileConnectedItem::onDNSWhileConnectedModeChanged(QVariant v)
{
    updateHeight(static_cast<DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE>(v.toInt()));

    if (curDNSWhileConnected_.type() != v.toInt())
    {
        curDNSWhileConnected_.setType(static_cast<DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE>(v.toInt()));

        // clear ip when robert is selected
        if (curDNSWhileConnected_.type() == DnsWhileConnectedInfo::ROBERT)
        {
            curDNSWhileConnected_.setIpAddress("");
            editBoxIP_->setText("");
        }

        emit dnsWhileConnectedInfoChanged(curDNSWhileConnected_);
    }
}

void DnsWhileConnectedItem::onDNSWhileConnectedIPChanged(QString v)
{
    if (curDNSWhileConnected_.ipAddress() != v)
    {
        curDNSWhileConnected_.setIpAddress(v);
        emit dnsWhileConnectedInfoChanged(curDNSWhileConnected_);
    }
}

void DnsWhileConnectedItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeightAndLinePos(value.toInt());
}

void DnsWhileConnectedItem::onLanguageChanged()
{
    QVariant dnsSelected = comboBoxDNS_->currentItem();

    comboBoxDNS_->clear();

    const QList<DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE> modes = DnsWhileConnectedInfo::allAvailableTypes();
    for (const DnsWhileConnectedInfo::DNS_WHILE_CONNECTED_TYPE &d : modes)
    {
        comboBoxDNS_->addItem(DnsWhileConnectedInfo::typeToString(d), static_cast<int>(d));
    }
    comboBoxDNS_->setCurrentItem(dnsSelected.toInt());
}

void DnsWhileConnectedItem::hideOpenPopups()
{
    comboBoxDNS_->hideMenu();
}

}
