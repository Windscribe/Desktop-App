#include "protocolgroup.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "tooltips/tooltiputil.h"
#include "tooltips/tooltipcontroller.h"

namespace PreferencesWindow {

ProtocolGroup::ProtocolGroup(ScalableGraphicsObject *parent, PreferencesHelper *preferencesHelper, const QString &title, const QString &path, const SelectionType type, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl), preferencesHelper_(preferencesHelper), title_(title), type_(type), isPortMapInitialized_(false)
{
    connect(preferencesHelper, &PreferencesHelper::portMapChanged, this, &ProtocolGroup::onPortMapChanged);

    if (type == SelectionType::COMBO_BOX) {
        connectionModeItem_ = new ComboBoxItem(this);
        connectionModeItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap(path));
        connect(connectionModeItem_, &ComboBoxItem::currentItemChanged, this, &ProtocolGroup::onAutomaticChanged);
        addItem(connectionModeItem_);
    } else {
        checkBoxEnable_ = new ToggleItem(this);
        checkBoxEnable_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap(path));
        checkBoxEnable_->setState(false);
        connect(checkBoxEnable_, &ToggleItem::stateChanged, this, &ProtocolGroup::onCheckBoxStateChanged);
        addItem(checkBoxEnable_);
    }

    protocolItem_ = new ComboBoxItem(this);
    connect(protocolItem_, &ComboBoxItem::currentItemChanged, this, &ProtocolGroup::onProtocolChanged);
    addItem(protocolItem_);

    portItem_ = new ComboBoxItem(this);
    connect(portItem_, &ComboBoxItem::currentItemChanged, this, &ProtocolGroup::onPortChanged);
    addItem(portItem_);

    hideItems(indexOf(protocolItem_), indexOf(portItem_), DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ProtocolGroup::onLanguageChanged);
    onLanguageChanged();
}

void ProtocolGroup::setConnectionSettings(const types::ConnectionSettings &cm)
{
    settings_ = cm;

    if (type_ == SelectionType::COMBO_BOX) {
        connectionModeItem_->setCurrentItem(settings_.isAutomatic() ? 0 : 1);
    } else {
        checkBoxEnable_->setState(!settings_.isAutomatic());
    }
    updateMode();
}

void ProtocolGroup::onPortMapChanged()
{
    protocolItem_->clear();
    portItem_->clear();

    const QVector<types::Protocol> protocols = preferencesHelper_->getAvailableProtocols();
    if (protocols.size() > 0) {
        isPortMapInitialized_ = true;
        for (auto pd : protocols) {
            protocolItem_->addItem(pd.toLongString(), pd.toInt());
        }
        protocolItem_->setCurrentItem(protocols.begin()->toInt());
        protocolItem_->setMaxMenuItemsShowing(protocols.count());
        updatePorts(*protocols.begin());
        protocolItem_->setClickable(true);
        portItem_->setClickable(true);
    } else {
        isPortMapInitialized_ = false;
        protocolItem_->setClickable(false);
        portItem_->setClickable(false);
    }
    updateMode();
}

void ProtocolGroup::onAutomaticChanged(QVariant value)
{
    settings_.setIsAutomatic(value.toInt() == 0);
    updateMode();
    emit connectionModePreferencesChanged(settings_);
}

void ProtocolGroup::onCheckBoxStateChanged(bool isChecked)
{
    settings_.setIsAutomatic(!isChecked);
    updateMode();
    emit connectionModePreferencesChanged(settings_);
}

void ProtocolGroup::onProtocolChanged(QVariant value)
{
    updatePorts(types::Protocol(value.toInt()));
    settings_.setProtocolAndPort(types::Protocol(protocolItem_->currentItem().toInt()), portItem_->currentItem().toInt());
    emit connectionModePreferencesChanged(settings_);
}

void ProtocolGroup::onPortChanged(QVariant value)
{
    settings_.setPort(portItem_->currentItem().toInt());
    emit connectionModePreferencesChanged(settings_);
}

void ProtocolGroup::updateMode()
{
    if (settings_.isAutomatic()) {
        hideItems(indexOf(protocolItem_), indexOf(portItem_));
    } else if (isPortMapInitialized_) {
        showItems(indexOf(protocolItem_), indexOf(portItem_));
        updatePorts(settings_.protocol().toInt());
        protocolItem_->setCurrentItem(settings_.protocol().toInt());
        portItem_->setCurrentItem(settings_.port());
    }
}

void ProtocolGroup::updatePorts(types::Protocol protocol)
{
    portItem_->clear();

    const QVector<uint> ports = preferencesHelper_->getAvailablePortsForProtocol(protocol);
    for (uint p : ports) {
        portItem_->addItem(QString::number(p), p);
    }
    portItem_->setCurrentItem(*ports.begin());
}

void ProtocolGroup::setTitle(const QString &title)
{
    title_ = title;
    onLanguageChanged();
}

void ProtocolGroup::onLanguageChanged()
{
    if (type_ == SelectionType::COMBO_BOX) {
        connectionModeItem_->setLabelCaption(title_);
        QList<QPair<QString, QVariant>> list;
        list << qMakePair(tr("Auto"), 0);
        list << qMakePair(tr("Manual"), 1);

        connectionModeItem_->setItems(list, settings_.isAutomatic() ? 0 : 1);
    } else {
        checkBoxEnable_->setCaption(title_);
    }

    protocolItem_->setLabelCaption(tr("Protocol"));
    portItem_->setLabelCaption(tr("Port"));
}

void ProtocolGroup::setDescription(const QString &description, const QString &descUrl)
{
    if (type_ == SelectionType::COMBO_BOX) {
        connectionModeItem_->setDescription(description, descUrl);
    } else {
        checkBoxEnable_->setDescription(description, descUrl);
    }
}

} // namespace PreferencesWindow
