#include "splittunnelinggroup.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>

#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "generalmessagecontroller.h"
#include "languagecontroller.h"

#if defined(Q_OS_MAC)
    #include "utils/macutils.h"
#elif defined(Q_OS_WINDOWS)
    #include "utils/logger.h"
    #include "utils/servicecontrolmanager.h"
#endif

extern QWidget *g_mainWindow;

namespace PreferencesWindow {

SplitTunnelingGroup::SplitTunnelingGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    activeCheckBox_ = new ToggleItem(this);
    activeCheckBox_->setState(true);
    activeCheckBox_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/SPLIT_TUNNELING"));
    connect(activeCheckBox_, &ToggleItem::stateChanged, this, &SplitTunnelingGroup::onActiveSwitchStateChanged);
    addItem(activeCheckBox_);

    modeComboBox_ = new ComboBoxItem(this);
    modeComboBox_->setCaptionFont(FontDescr(12, false));
    connect(modeComboBox_, &ComboBoxItem::currentItemChanged, this, &SplitTunnelingGroup::onCurrentModeChanged);
    addItem(modeComboBox_);

#ifdef Q_OS_WIN
    appsLinkItem_ = new LinkItem(this, LinkItem::LinkType::SUBPAGE_LINK);
    connect(appsLinkItem_, &LinkItem::clicked, this, &SplitTunnelingGroup::appsPageClick);
    addItem(appsLinkItem_);
#else
    appsLinkItem_ = nullptr;
#endif

    addressesLinkItem_ = new LinkItem(this, LinkItem::LinkType::SUBPAGE_LINK);
    connect(addressesLinkItem_, &LinkItem::clicked, this, &SplitTunnelingGroup::addressesPageClick);
    addItem(addressesLinkItem_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &SplitTunnelingGroup::onLanguageChanged);
    onLanguageChanged();

    updateScaling();
}

void SplitTunnelingGroup::setSettings(types::SplitTunnelingSettings settings)
{
    settings_ = settings;
    activeCheckBox_->setState(settings.active);
    updateUIState(settings.active);
    if (modeComboBox_->hasItems()) {
        modeComboBox_->setCurrentItem(settings.mode);
    }
    onCurrentModeChanged(settings.mode);
}

void SplitTunnelingGroup::onActiveSwitchStateChanged(bool checked)
{
#if defined(Q_OS_WINDOWS)
    if (checked) {
        try {
            // Note: accessing the Windows SCM could potentially be a slow operation.  Be careful if
            // moving/replicating this logic in a code path invoked during app startup.
            wsl::ServiceControlManager manager;
            manager.openSCM(SC_MANAGER_CONNECT);
            if (!manager.isServiceInstalled(L"WindscribeSplitTunnel")) {
                checked = false;
                activeCheckBox_->setState(false);
                GeneralMessageController::instance().showMessage(
                    "WARNING_YELLOW",
                    tr("Service Not Installed"),
                    tr("The split tunneling driver is not installed.  To enable this feature, try"
                       " reinstalling the Windscribe application.\n\nIf the reinstall does not help, please"
                       " contact Windscribe support for assistance."),
                    GeneralMessageController::tr(GeneralMessageController::kOk),
                    "",
                    "",
                    std::function<void(bool)>(nullptr),
                    std::function<void(bool)>(nullptr),
                    std::function<void(bool)>(nullptr),
                    GeneralMessage::kFromPreferences);
            }
        }
        catch (std::system_error& ex) {
            qCDebug(LOG_PREFERENCES) << "SplitTunnelingGroup::onActiveSwitchStateChanged -" << ex.what();
        }
    }
#endif

    updateUIState(checked);
}

void SplitTunnelingGroup::onCurrentModeChanged(QVariant value)
{
    SPLIT_TUNNELING_MODE mode = (SPLIT_TUNNELING_MODE)value.toInt();
    settings_.mode = mode;
    emit settingsChanged(settings_);

    updateDescription();
}

void SplitTunnelingGroup::setAppsCount(int count)
{
    QString num(QString("%1").arg(count));
    if (appsLinkItem_) {
        appsLinkItem_->setLinkText(num);
    }
}

void SplitTunnelingGroup::setAddressesCount(int count)
{
    QString num(QString("%1").arg(count));
    addressesLinkItem_->setLinkText(num);
}

void SplitTunnelingGroup::updateDescription()
{
    switch(settings_.mode) {
        case SPLIT_TUNNELING_MODE_EXCLUDE:
#ifdef Q_OS_WIN
            setDescription(tr("Selected apps, IPs, and hostnames will not go through Windscribe when connected."));
#else
            setDescription(tr("Selected IPs and hostnames will not go through Windscribe when connected."));
#endif
            break;
        case SPLIT_TUNNELING_MODE_INCLUDE:
#ifdef Q_OS_WIN
            setDescription(tr("Only selected apps, IPs, and hostnames will go through Windscribe when connected."));
#else
            setDescription(tr("Only selected IPs and hostnames will go through Windscribe when connected."));
#endif
            break;
        default:
            break;
    }
}

void SplitTunnelingGroup::setEnabled(bool enabled)
{
    activeCheckBox_->setEnabled(enabled);
}

void SplitTunnelingGroup::setActive(bool active)
{
    activeCheckBox_->setState(active);
    updateUIState(active);
}

void SplitTunnelingGroup::updateUIState(bool active)
{
    if (settings_.active != active) {
        settings_.active = active;
        emit settingsChanged(settings_);
    }

    if (active) {
        showDescription();
        showItems(indexOf(modeComboBox_), size() - 1);
    }
    else {
        hideDescription();
        hideItems(indexOf(modeComboBox_), size() - 1);
    }
}

void SplitTunnelingGroup::onLanguageChanged()
{
    activeCheckBox_->setCaption(tr("Split Tunneling"));
    modeComboBox_->setLabelCaption(tr("Mode"));
    QList<QPair<QString, QVariant>> list;
    list << qMakePair(tr("Exclusive"), SPLIT_TUNNELING_MODE_EXCLUDE);
    list << qMakePair(tr("Inclusive"), SPLIT_TUNNELING_MODE_INCLUDE);
    modeComboBox_->setItems(list, settings_.mode);

#ifdef Q_OS_WIN
    appsLinkItem_->setTitle(tr("Apps"));
#endif
    addressesLinkItem_->setTitle(tr("IPs & Hostnames"));

    updateDescription();
}

}
