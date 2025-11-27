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
  : PreferenceGroup(parent, desc, descUrl), isLocalDnsAvailable_(false), isControldRequestInProgress_(false), controldLastFetchResult_(CONTROLD_FETCH_SUCCESS)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape | QGraphicsItem::ItemIsFocusable);

    comboBoxDns_ = new ComboBoxItem(this);
    comboBoxDns_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/CONNECTED_DNS"));
    connect(comboBoxDns_, &ComboBoxItem::currentItemChanged, this, &ConnectedDnsGroup::onConnectedDnsModeChanged);
    addItem(comboBoxDns_);

    editBoxControldApiKey_ = new VerticalEditBoxItem(this);
    connect(editBoxControldApiKey_, &VerticalEditBoxItem::textChanged, this, &ConnectedDnsGroup::onControldApiKeyChanged);
    connect(editBoxControldApiKey_, &VerticalEditBoxItem::refreshButtonClicked, this, &ConnectedDnsGroup::onControldApiKeyRefreshClick);
    addItem(editBoxControldApiKey_);

    comboBoxControldDevice_ = new ComboBoxItem(this);
    connect(comboBoxControldDevice_, &ComboBoxItem::currentItemChanged, this, &ConnectedDnsGroup::onControldDeviceChanged);
    addItem(comboBoxControldDevice_);

    editBoxUpstream1_ = new VerticalEditBoxItem(this);
    connect(editBoxUpstream1_, &VerticalEditBoxItem::textChanged, this, &ConnectedDnsGroup::onUpstream1Changed);
    addItem(editBoxUpstream1_);

    splitDnsCheckBox_ = new ToggleItem(this);
    splitDnsCheckBox_->setState(false);
    splitDnsCheckBox_->setEnabled(false);
    connect(splitDnsCheckBox_, &ToggleItem::stateChanged, this, &ConnectedDnsGroup::onSplitDnsStateChanged);
    addItem(splitDnsCheckBox_, true);

    editBoxUpstream2_ = new VerticalEditBoxItem(this);
    connect(editBoxUpstream2_, &VerticalEditBoxItem::textChanged, this, &ConnectedDnsGroup::onUpstream2Changed);
    addItem(editBoxUpstream2_);

    domainsItem_ = new LinkItem(this, LinkItem::LinkType::SUBPAGE_LINK);
    connect(domainsItem_, &LinkItem::clicked, this, &ConnectedDnsGroup::onDomainsClick);

    addItem(domainsItem_);

    hideItems(indexOf(editBoxControldApiKey_), indexOf(domainsItem_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

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
        bool apiKeyChanged = settings_.controldApiKey != dns.controldApiKey;

        settings_ = dns;

        // update inner widgets
        if (dns.type == CONNECTED_DNS_TYPE_AUTO)
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_AUTO);
        else if (dns.type == CONNECTED_DNS_TYPE_LOCAL)
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_LOCAL);
        else if (dns.type == CONNECTED_DNS_TYPE_CONTROLD)
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_CONTROLD);
        else
            comboBoxDns_->setCurrentItem(CONNECTED_DNS_TYPE_CUSTOM);

        // Check if API key changed
        editBoxControldApiKey_->setText(dns.controldApiKey);

        // If API key changed, clear devices and fetch new ones
        if (apiKeyChanged && !dns.controldApiKey.isEmpty()) {
            settings_.controldDevices.clear();
            comboBoxControldDevice_->clear();
            fetchDevices(dns.controldApiKey);
        }
        // If we have an API key but no devices, fetch them
        else if (!dns.controldApiKey.isEmpty() && dns.controldDevices.isEmpty()) {
            fetchDevices(dns.controldApiKey);
        } else if (!dns.controldDevices.isEmpty()) {
            // Populate Control D devices combo box
            QList<QPair<QString, QVariant>> devicesList;
            for (const auto& device : dns.controldDevices) {
                devicesList << qMakePair(device.first, QVariant(device.second));
            }
            // Find the device that matches upstream1
            QVariant selectedDevice = devicesList.first().second;
            for (const auto& device : devicesList) {
                if (device.second.toString() == dns.upStream1) {
                    selectedDevice = device.second;
                    break;
                }
            }
            comboBoxControldDevice_->setItems(devicesList, selectedDevice);
        }

        splitDnsCheckBox_->setState(dns.isSplitDns);

        if (dns.type == CONNECTED_DNS_TYPE_LOCAL) {
            editBoxUpstream1_->setText("127.0.0.1");
            settings_.upStream1 = "127.0.0.1";
            onUpstream1Changed("127.0.0.1");
        } else {
            editBoxUpstream1_->setText(dns.upStream1);
            onUpstream1Changed(dns.upStream1);
        }

        editBoxUpstream2_->setText(dns.upStream2);
        domainsItem_->setLinkText(QString::number(dns.hostnames.count()));

        updateMode();
    }
}

void ConnectedDnsGroup::updateMode()
{
    if (settings_.type == CONNECTED_DNS_TYPE_AUTO) {
        hideItems(indexOf(editBoxControldApiKey_), indexOf(domainsItem_));
    } else if (settings_.type == CONNECTED_DNS_TYPE_LOCAL) {
        hideItems(indexOf(editBoxControldApiKey_), indexOf(comboBoxControldDevice_));
        showItems(indexOf(editBoxUpstream1_), indexOf(editBoxUpstream1_));
        hideItems(indexOf(splitDnsCheckBox_), indexOf(domainsItem_));
        editBoxUpstream1_->setEnabled(false);
    } else if (settings_.type == CONNECTED_DNS_TYPE_CONTROLD) {
        hideItems(indexOf(editBoxUpstream1_), indexOf(domainsItem_));
        showItems(indexOf(editBoxControldApiKey_), indexOf(editBoxControldApiKey_));
        if (settings_.controldApiKey.isEmpty() || controldLastFetchResult_ != CONTROLD_FETCH_SUCCESS) {
            hideItems(indexOf(comboBoxControldDevice_), indexOf(domainsItem_));
        } else {
            showItems(indexOf(comboBoxControldDevice_), indexOf(comboBoxControldDevice_));
            comboBoxControldDevice_->setEnabled(!settings_.controldDevices.isEmpty());
            if (settings_.isSplitDns) {
                showItems(indexOf(splitDnsCheckBox_), indexOf(domainsItem_));
            } else {
                showItems(indexOf(splitDnsCheckBox_), indexOf(splitDnsCheckBox_));
            }
        }
    } else  {
        hideItems(indexOf(editBoxControldApiKey_), indexOf(comboBoxControldDevice_));
        editBoxUpstream1_->setEnabled(true);
        if (settings_.isSplitDns) {
            showItems(indexOf(editBoxUpstream1_), indexOf(domainsItem_));
        } else {
            showItems(indexOf(editBoxUpstream1_), indexOf(splitDnsCheckBox_));
        }
    }
}

void ConnectedDnsGroup::onConnectedDnsModeChanged(QVariant v)
{
    if (settings_.type != v.toInt())
    {
        CONNECTED_DNS_TYPE oldType = settings_.type;
        settings_.type = (CONNECTED_DNS_TYPE)v.toInt();

        if (settings_.type == CONNECTED_DNS_TYPE_LOCAL) {
            editBoxUpstream1_->setText("127.0.0.1");
            settings_.upStream1 = "127.0.0.1";
        } else if (oldType == CONNECTED_DNS_TYPE_LOCAL) {
            editBoxUpstream1_->setText("");
            settings_.upStream1 = "";
            onUpstream1Changed("");
            editBoxUpstream1_->setEnabled(true);
        }

        if (v.toInt() == CONNECTED_DNS_TYPE_CUSTOM) {
            checkDnsLeak(settings_.upStream1, settings_.isSplitDns ? settings_.upStream2 : "");
        }

        updateMode();
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onControldApiKeyChanged(QString v)
{
    if (settings_.controldApiKey != v) {
        settings_.controldApiKey = v;

        settings_.controldDevices.clear();
        settings_.upStream1.clear();
        comboBoxControldDevice_->clear();
        editBoxUpstream1_->setText("");
        editBoxControldApiKey_->setError("");
        editBoxControldApiKey_->setRefreshButtonVisible(false);
        onUpstream1Changed("");

        if (!v.isEmpty()) {
            controldLastFetchResult_ = CONTROLD_FETCH_SUCCESS;
            fetchDevices(v);
        }

        updateMode();

        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::onControldApiKeyRefreshClick()
{
    editBoxControldApiKey_->setError("");
    editBoxControldApiKey_->setRefreshButtonVisible(false);
    controldLastFetchResult_ = CONTROLD_FETCH_SUCCESS;

    settings_.controldDevices.clear();
    settings_.upStream1.clear();
    comboBoxControldDevice_->clear();
    editBoxUpstream1_->setText("");
    onUpstream1Changed("");

    updateMode();

    if (!settings_.controldApiKey.isEmpty()) {
        fetchDevices(settings_.controldApiKey);
    }
}

void ConnectedDnsGroup::onControldDeviceChanged(QVariant v)
{
    QString resolver = v.toString();
    if (settings_.upStream1 != resolver) {
        onUpstream1Changed(resolver);
    }
}

void ConnectedDnsGroup::onUpstream1Changed(QString v)
{
    if (v.isEmpty()) {
        splitDnsCheckBox_->setState(false);
        onSplitDnsStateChanged(false);
    }
    splitDnsCheckBox_->setEnabled(!v.isEmpty());

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
        } else  { // show additional items
            showItems(indexOf(editBoxUpstream2_), indexOf(domainsItem_));
        }

        settings_.isSplitDns = checked;
        emit connectedDnsInfoChanged(settings_);
    }
}

void ConnectedDnsGroup::fetchDevices(const QString &apiKey)
{
    if (isControldRequestInProgress_) {
        return;
    }

    isControldRequestInProgress_ = true;
    comboBoxControldDevice_->setInProgress(true);

    emit fetchControldDevices(apiKey);
}

void ConnectedDnsGroup::onControldDevicesFetched(CONTROLD_FETCH_RESULT result, const QList<QPair<QString, QString>> &devices)
{
    isControldRequestInProgress_ = false;
    editBoxControldApiKey_->setPrompt(tr("API Key"));
    comboBoxControldDevice_->setInProgress(false);
    controldLastFetchResult_ = result;

    if (result == CONTROLD_FETCH_NETWORK_ERROR) {
        qCDebug(LOG_BASIC) << "Control D devices fetched: network error";
        updateMode();
        onLanguageChanged();
        return;
    } else if (result == CONTROLD_FETCH_AUTH_ERROR) {
        qCDebug(LOG_BASIC) << "Control D devices fetched: auth error";
        updateMode();
        onLanguageChanged();
        return;
    } else if (result == CONTROLD_FETCH_SUCCESS) {
        editBoxControldApiKey_->setError("");
        editBoxControldApiKey_->setRefreshButtonVisible(false);
    }

    settings_.controldDevices = devices;

    QList<QPair<QString, QVariant>> devicesList;
    for (const auto& device : settings_.controldDevices) {
        devicesList << qMakePair(device.first, QVariant(device.second));
    }
    if (!devicesList.isEmpty()) {
        QVariant selectedDevice;

        if (settings_.upStream1.isEmpty()) {
            // No upstream1 set, use first device
            selectedDevice = devicesList.first().second;
            settings_.upStream1 = selectedDevice.toString();
        } else {
            // upstream1 already exists, try to find matching device
            bool found = false;
            for (const auto& device : devicesList) {
                if (device.second.toString() == settings_.upStream1) {
                    selectedDevice = device.second;
                    found = true;
                    break;
                }
            }
            // If not found in list, use first device
            if (!found) {
                selectedDevice = devicesList.first().second;
                settings_.upStream1 = selectedDevice.toString();
            } else {
                selectedDevice = settings_.upStream1;
            }
        }

        comboBoxControldDevice_->setItems(devicesList, selectedDevice);
        comboBoxControldDevice_->setEnabled(true);
        editBoxUpstream1_->setText(selectedDevice.toString());
        onUpstream1Changed(selectedDevice.toString());
    } else {
        editBoxUpstream1_->setText("");
        onUpstream1Changed("");
    }

    updateMode();
    emit connectedDnsInfoChanged(settings_);
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
    comboBoxControldDevice_->hideMenu();
}

void ConnectedDnsGroup::populateDnsTypes(bool isLocalDnsAvailable)
{
    QList<QPair<QString, QVariant>> list;
    QList<CONNECTED_DNS_TYPE> types = types::ConnectedDnsInfo::allAvailableTypes();

    for (const auto t : types) {
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

    editBoxControldApiKey_->setCaption(tr("API Key"));
    editBoxControldApiKey_->setPrompt(tr("API Key"));
    comboBoxControldDevice_->setLabelCaption(tr("Upstream 1"));

    if (controldLastFetchResult_ == CONTROLD_FETCH_NETWORK_ERROR) {
        editBoxControldApiKey_->setError(tr("Failed to reach Control D API."));
        editBoxControldApiKey_->setRefreshButtonVisible(true);
    } else if (controldLastFetchResult_ == CONTROLD_FETCH_AUTH_ERROR) {
        editBoxControldApiKey_->setError(tr("Please provide a valid Control D API Key."));
        editBoxControldApiKey_->setRefreshButtonVisible(false);
    }

    editBoxUpstream1_->setCaption(tr("Upstream 1"));
    editBoxUpstream1_->setPrompt(tr("IP/DNS-over-HTTPS/TLS"));
    editBoxUpstream2_->setCaption(tr("Upstream 2"));
    editBoxUpstream2_->setPrompt(tr("IP/DNS-over-HTTPS/TLS"));
    splitDnsCheckBox_->setCaption(tr("Split DNS"));
    domainsItem_->setTitle(tr("Domains"));
}

void ConnectedDnsGroup::setDescription(const QString &desc, const QString &descUrl)
{
    comboBoxDns_->setDescription(desc, descUrl);
}

}
