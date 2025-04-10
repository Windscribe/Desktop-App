#include "connecteddnsgroup.h"

#include "dpiscalemanager.h"
#include "generalmessagecontroller.h"
#include "languagecontroller.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/ipvalidation.h"

#if defined(Q_OS_WIN)
#include "utils/winutils.h"
#endif

namespace PreferencesWindow {

ConnectedDnsGroup::ConnectedDnsGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl), isLocalDnsAvailable_(false)
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
        if (dns.type == CONNECTED_DNS_TYPE_AUTO)
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_AUTO);
#if defined(Q_OS_WIN)
        else if (dns.type == CONNECTED_DNS_TYPE_FORCED)
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_FORCED);
#endif
        else if (dns.type == CONNECTED_DNS_TYPE_LOCAL)
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_LOCAL);
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
    if (settings_.type == CONNECTED_DNS_TYPE_AUTO || settings_.type == CONNECTED_DNS_TYPE_FORCED || settings_.type == CONNECTED_DNS_TYPE_LOCAL) {
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
        if (v.toInt() == CONNECTED_DNS_TYPE_CUSTOM) {
            checkDnsLeak(settings_.upStream1, settings_.isSplitDns ? settings_.upStream2 : "");
        }
        settings_.type = (CONNECTED_DNS_TYPE)v.toInt();
        updateMode();
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onUpstream1Changed(QString v)
{
    if (settings_.upStream1 != v) {
        checkDnsLeak(v);
        settings_.upStream1 = v;
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onUpstream2Changed(QString v)
{
    if (settings_.upStream2 != v) {
        checkDnsLeak(v);
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
            checkDnsLeak(settings_.upStream2);
            showItems(indexOf(editBoxUpstream1_), indexOf(domainsItem_));
        }

        settings_.isSplitDns = checked;
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::checkDnsLeak(const QString &v1, const QString &v2)
{
    if (IpValidation::isLocalIp(v1) || IpValidation::isLocalIp(v2)) {
        GeneralMessageController::instance().showMessage(
            "WARNING_YELLOW",
            tr("DNS leak detected"),
            tr("Using a LAN or local IP address for connected DNS will result in a DNS leak.  We strongly recommend using ROBERT or a public DNS server."),
            GeneralMessageController::tr(GeneralMessageController::kOk),
            "",
            "",
            std::function<void(bool)>(nullptr),
            std::function<void(bool)>(nullptr),
            std::function<void(bool)>(nullptr),
            GeneralMessage::kFromPreferences);
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

void ConnectedDnsGroup::populateDnsTypes(bool isLocalDnsAvailable)
{
    QList<QPair<QString, QVariant>> list;
    QList<CONNECTED_DNS_TYPE> types = types::ConnectedDnsInfo::allAvailableTypes();

    for (const auto t : types) {
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
        if (t == CONNECTED_DNS_TYPE_FORCED)
            continue; // only available on Windows
#else // Windows
        if (t == CONNECTED_DNS_TYPE_FORCED && !WinUtils::isDohSupported())
            continue;
#endif
        if (t != CONNECTED_DNS_TYPE_LOCAL) {
            list << qMakePair(CONNECTED_DNS_TYPE_toString(t), t);
        }
    }

    // Add Local DNS option if available, or if it's already set to local
    if (isLocalDnsAvailable || settings_.type == CONNECTED_DNS_TYPE_LOCAL) {
        list << qMakePair(CONNECTED_DNS_TYPE_toString(CONNECTED_DNS_TYPE_LOCAL), CONNECTED_DNS_TYPE_LOCAL);
    }

    comboBoxDns_->setItems(list, settings_.type);
}

void ConnectedDnsGroup::setLocalDnsAvailable(bool available)
{
    isLocalDnsAvailable_ = available;
    populateDnsTypes(available);

    // Note: once the user has selected Local DNS, it will remain selected even if it's not available,
    // unless the user attempts to connect.  At that point, the engine will emit a signal,
    // and MainWindow will change the setting to Auto.
    // However, we don't want to change it without explicitly showing the user an alert, especially
    // since there may be false positives (e.g. opening preferences immediately after turning on the firewall).
}

void ConnectedDnsGroup::onLanguageChanged()
{
    comboBoxDns_->setLabelCaption(tr("Connected DNS"));
    populateDnsTypes(isLocalDnsAvailable_);

    editBoxUpstream1_->setCaption(tr("Upstream 1"));
    editBoxUpstream1_->setPrompt(tr("IP/DNS-over-HTTPS/TLS"));
    editBoxUpstream2_->setCaption(tr("Upstream 2"));
    editBoxUpstream2_->setPrompt(tr("IP/DNS-over-HTTPS/TLS"));
    splitDnsCheckBox_->setCaption(tr("Split DNS"));
    domainsItem_->setTitle(tr("Domains"));
}

}
