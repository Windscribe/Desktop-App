#include "dnswhileconnecteditem.h"

namespace PreferencesWindow {

DNSWhileConnectedItem::DNSWhileConnectedItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50),
   comboBoxDNS_(nullptr)
   ,isExpanded_(false)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    comboBoxDNS_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "DNS While Connected"), "", 50, Qt::transparent, 0, false);
    QList<DNSWhileConnectedType> modes = DNSWhileConnectedType::allAvailableTypes();
    foreach (const DNSWhileConnectedType & d, modes)
    {
        comboBoxDNS_->addItem(d.toString(), static_cast<int>(d.type()));
    }
    comboBoxDNS_->setCurrentItem(modes.begin()->type());
    connect(comboBoxDNS_, SIGNAL(currentItemChanged(QVariant)), SLOT(onDNSWhileConnectedModeChanged(QVariant)));

    editBoxIP_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "IP Address"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "Enter IP"), false);
    connect(editBoxIP_, SIGNAL(textChanged(QString)), SLOT(onDNSWhileConnectedIPChanged(QString)));

    editBoxIP_->setPos(0, COLLAPSED_HEIGHT);

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegExp ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    editBoxIP_->setValidator(ipValidator);

    dividerLine_ = new DividerLine(this, 276);
    dividerLine_->setPos(24, 50 - dividerLine_->boundingRect().height());

    expandEnimation_.setStartValue(COLLAPSED_HEIGHT);
    expandEnimation_.setEndValue(EXPANDED_HEIGHT);
    expandEnimation_.setDuration(150);
    connect(&expandEnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandAnimationValueChanged(QVariant)));
}

void DNSWhileConnectedItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void DNSWhileConnectedItem::updateScaling()
{
    comboBoxDNS_->updateScaling();
}

/*void DNSWhileConnectedItem::setDNSWhileConnected(const DNSWhileConnected &dns)
{
    curDNSWhileConnected_ = dns;

    if (dns.type == DNSWhileConnectedType::ROBERT)
    {
        comboBoxDNS_->setCurrentItem(DNSWhileConnectedType::ROBERT);
        isExpanded_ = false;
    }
    else
    {
        editBoxIP_->setText(dns.ipAddress);
        comboBoxDNS_->setCurrentItem(DNSWhileConnectedType::CUSTOM);
        isExpanded_ = true;
    }
}*/

void DNSWhileConnectedItem::onDNSWhileConnectedModeChanged(QVariant v)
{
    if (v.toInt() == DNSWhileConnectedType::CUSTOM && !isExpanded_)
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

    /*if (curDNSWhileConnected_.type.type() != v.toInt())
    {
        curDNSWhileConnected_.type = v.toInt();
        emit dnsWhileConnectedChanged(curDNSWhileConnected_);
    }*/
}

void DNSWhileConnectedItem::onDNSWhileConnectedIPChanged(QString v)
{
    /*if (curDNSWhileConnected_.ipAddress != v)
    {
        curDNSWhileConnected_.ipAddress = v;
        emit dnsWhileConnectedChanged(curDNSWhileConnected_);
    }*/
}

void DNSWhileConnectedItem::onExpandAnimationValueChanged(const QVariant &value)
{
    setHeight(value.toInt());

    dividerLine_->setPos(24, value.toInt() - dividerLine_->boundingRect().height());
}

void DNSWhileConnectedItem::onLanguageChanged()
{
    QVariant dnsSelected = comboBoxDNS_->currentItem();

    comboBoxDNS_->clear();

    QList<DNSWhileConnectedType> modes = DNSWhileConnectedType::allAvailableTypes();
    foreach (const DNSWhileConnectedType & d, modes)
    {
        comboBoxDNS_->addItem(d.toString(), static_cast<int>(d.type()));
    }
    comboBoxDNS_->setCurrentItem(dnsSelected.toInt());
}

void DNSWhileConnectedItem::hideOpenPopups()
{
    comboBoxDNS_->hideMenu();
}

}
