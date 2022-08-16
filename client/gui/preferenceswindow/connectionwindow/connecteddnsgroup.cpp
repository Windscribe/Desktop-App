#include "connecteddnsgroup.h"

#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"

namespace PreferencesWindow {

ConnectedDnsGroup::ConnectedDnsGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    comboBoxDns_ = new ComboBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::ComboBoxItem", "Connected DNS"), "");
    const QList<CONNECTED_DNS_TYPE> modes = types::ConnectedDnsInfo::allAvailableTypes();
    for (const CONNECTED_DNS_TYPE &d : modes)
    {
        comboBoxDns_->addItem(types::ConnectedDnsInfo::typeToString(d), static_cast<int>(d));
    }
    comboBoxDns_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/CONNECTED_DNS"));
    comboBoxDns_->setCurrentItem(modes.first());
    connect(comboBoxDns_, &ComboBoxItem::currentItemChanged, this, &ConnectedDnsGroup::onConnectedDnsModeChanged);
    addItem(comboBoxDns_);

    editBoxIp_ = new EditBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "IP Address"), QT_TRANSLATE_NOOP("PreferencesWindow::EditBoxItem", "IP address"));
    connect(editBoxIp_, &EditBoxItem::textChanged, this, &ConnectedDnsGroup::onConnectedDnsIpChanged);
    addItem(editBoxIp_);
    hideItems(indexOf(editBoxIp_), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegularExpression ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);
    editBoxIp_->setValidator(ipValidator);
}

bool ConnectedDnsGroup::hasItemWithFocus()
{
    return editBoxIp_->lineEditHasFocus();
}

void ConnectedDnsGroup::setConnectedDnsInfo(const types::ConnectedDnsInfo &dns)
{
    if (dns != settings_)
    {
        settings_ = dns;

        // update inner widgets
        if (dns.type() == CONNECTED_DNS_TYPE_ROBERT)
        {
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_ROBERT);
        }
        else
        {
            editBoxIp_->setText(dns.ipAddress());
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_CUSTOM);
        }
        updateMode();
    }
}

void ConnectedDnsGroup::updateMode()
{
    if (settings_.type() == CONNECTED_DNS_TYPE_ROBERT)
    {
        settings_.setIpAddress("");
        editBoxIp_->setText("");
        hideItems(indexOf(editBoxIp_));
    }
    else
    {
        showItems(indexOf(editBoxIp_));
    }
}

void ConnectedDnsGroup::onConnectedDnsModeChanged(QVariant v)
{
    if (settings_.type() != v.toInt())
    {
        settings_.setType((CONNECTED_DNS_TYPE)v.toInt());
        updateMode();
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onConnectedDnsIpChanged(QString v)
{
    if (settings_.ipAddress() != v)
    {
        settings_.setIpAddress(v);
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onLanguageChanged()
{
    QVariant dnsSelected = comboBoxDns_->currentItem();

    comboBoxDns_->clear();

    const QList<CONNECTED_DNS_TYPE> modes = types::ConnectedDnsInfo::allAvailableTypes();
    for (const CONNECTED_DNS_TYPE &d : modes)
    {
        comboBoxDns_->addItem(types::ConnectedDnsInfo::typeToString(d), static_cast<int>(d));
    }
    comboBoxDns_->setCurrentItem(dnsSelected.toInt());
}

void ConnectedDnsGroup::hideOpenPopups()
{
    comboBoxDns_->hideMenu();
}

}
