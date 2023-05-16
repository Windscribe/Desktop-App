#include "connecteddnsgroup.h"

#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "graphicresources/imageresourcessvg.h"

namespace PreferencesWindow {

ConnectedDnsGroup::ConnectedDnsGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    comboBoxDns_ = new ComboBoxItem(this);
    comboBoxDns_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/CONNECTED_DNS"));
    connect(comboBoxDns_, &ComboBoxItem::currentItemChanged, this, &ConnectedDnsGroup::onConnectedDnsModeChanged);
    addItem(comboBoxDns_);

    editBoxUpstream1_ = new VerticalEditBoxItem(this);
    connect(editBoxUpstream1_, &VerticalEditBoxItem::textChanged, this, &ConnectedDnsGroup::onUpstream1Changed);
    addItem(editBoxUpstream1_);

    splitDnsCheckBox_ = new ToggleItem(this);
    splitDnsCheckBox_->setState(false);
    connect(splitDnsCheckBox_, &ToggleItem::stateChanged, this, &ConnectedDnsGroup::onSplitDnsStateChanged);
    addItem(splitDnsCheckBox_, true);

    editBoxUpstream2_ = new VerticalEditBoxItem(this);
    connect(editBoxUpstream2_, &VerticalEditBoxItem::textChanged, this, &ConnectedDnsGroup::onUpstream2Changed);
    addItem(editBoxUpstream2_);

    domainsItem_ = new LinkItem(this, LinkItem::LinkType::SUBPAGE_LINK);
    connect(domainsItem_, &LinkItem::clicked, this, &ConnectedDnsGroup::onDomainsClick);

    addItem(domainsItem_);

    hideItems(indexOf(editBoxUpstream1_), indexOf(domainsItem_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ConnectedDnsGroup::onLanguageChanged);
    onLanguageChanged();
}

bool ConnectedDnsGroup::hasItemWithFocus()
{
    return editBoxUpstream1_->lineEditHasFocus();
}

void ConnectedDnsGroup::setConnectedDnsInfo(const types::ConnectedDnsInfo &dns)
{
    if (dns != settings_)
    {
        settings_ = dns;

        // update inner widgets
        if (dns.type == CONNECTED_DNS_TYPE_ROBERT)
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_ROBERT);
        else
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_CUSTOM);

        editBoxUpstream1_->setText(dns.upStream1);
        editBoxUpstream2_->setText(dns.upStream2);
        splitDnsCheckBox_->setState(dns.isSplitDns);
        domainsItem_->setLinkText(QString::number(dns.hostnames.count()));

        updateMode();
    }
}

void ConnectedDnsGroup::updateMode()
{
    if (settings_.type == CONNECTED_DNS_TYPE_ROBERT) {
        hideItems(indexOf(editBoxUpstream1_), indexOf(domainsItem_));
    } else  {
        if (settings_.isSplitDns)
            showItems(indexOf(editBoxUpstream1_), indexOf(domainsItem_));
        else
            showItems(indexOf(editBoxUpstream1_), indexOf(splitDnsCheckBox_));
    }
}

void ConnectedDnsGroup::onConnectedDnsModeChanged(QVariant v)
{
    if (settings_.type != v.toInt())
    {
        settings_.type = (CONNECTED_DNS_TYPE)v.toInt();
        updateMode();
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onUpstream1Changed(QString v)
{
    if (settings_.upStream1 != v) {
        settings_.upStream1 = v;
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onUpstream2Changed(QString v)
{
    if (settings_.upStream2 != v) {
        settings_.upStream2 = v;
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onSplitDnsStateChanged(bool checked)
{
    if (settings_.isSplitDns != checked) {

        // hide additional items
        if (settings_.isSplitDns && !checked) {
            hideItems(indexOf(editBoxUpstream2_), indexOf(domainsItem_));
        } else  {        // show additional items
            showItems(indexOf(editBoxUpstream1_), indexOf(domainsItem_));
        }

        settings_.isSplitDns = checked;
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onDomainsClick()
{
    emit domainsClick(settings_.hostnames);
}

void ConnectedDnsGroup::hideOpenPopups()
{
    comboBoxDns_->hideMenu();
}

void ConnectedDnsGroup::onLanguageChanged()
{
    comboBoxDns_->setLabelCaption(tr("Connected DNS"));
    QList<CONNECTED_DNS_TYPE> types = types::ConnectedDnsInfo::allAvailableTypes();
    QList<QPair<QString, QVariant>> list;
    for (const auto t : types) {
        list << qMakePair(types::ConnectedDnsInfo::typeToString(t), t);
    }
    comboBoxDns_->setItems(list, settings_.type);

    editBoxUpstream1_->setCaption(tr("Upstream 1"));
    editBoxUpstream1_->setPrompt(tr("IP/DNS-over-HTTPS/TLS"));
    editBoxUpstream2_->setCaption(tr("Upstream 2"));
    editBoxUpstream2_->setPrompt(tr("IP/DNS-over-HTTPS/TLS"));
    splitDnsCheckBox_->setCaption(tr("Split DNS"));
    domainsItem_->setTitle(tr("Domains"));
}

}
