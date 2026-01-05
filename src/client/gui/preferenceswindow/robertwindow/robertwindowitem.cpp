#include "robertwindowitem.h"

#include <QTextDocument>
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "utils/hardcodedsettings.h"

namespace PreferencesWindow {

RobertWindowItem::RobertWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent), loggedIn_(false), isError_(false), loading_(false), manageRulesItem_(nullptr)
{
    Q_UNUSED(preferences);
    Q_UNUSED(preferencesHelper);

    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN_Y);

    desc_ = new PreferenceGroup(this);
    addItem(desc_);

    // Loading items
    loadingIcon_ = new LoadingIconItem(this, ":/gif/windscribe_spinner.gif", kLoadingIconSize, kLoadingIconSize);
    loadingIcon_->setOpacity(OPACITY_SIXTY);

    // Item for case where we could not get status from server
    errorMessage_ = new QGraphicsTextItem(this);
    errorMessage_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    errorMessage_->setDefaultTextColor(Qt::white);
    errorMessage_->setTextWidth(125);
    errorMessage_->document()->setDefaultTextOption(QTextOption(Qt::AlignHCenter));

    // Below items for case when not logged in
    loginPrompt_ = new QGraphicsTextItem(this);
    loginPrompt_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    loginPrompt_->setDefaultTextColor(Qt::white);
    loginPrompt_->setTextWidth(125);
    loginPrompt_->document()->setDefaultTextOption(QTextOption(Qt::AlignHCenter));

    loginButton_ = new CommonGraphics::BubbleButton(this, CommonGraphics::BubbleButton::kOutline, 69, 24, 12);
    loginButton_->setFont(FontDescr(14, QFont::Normal));
    connect(loginButton_, &CommonGraphics::BubbleButton::clicked, this, &RobertWindowItem::accountLoginClick);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &RobertWindowItem::onLanguageChanged);
    onLanguageChanged();

    updatePositions();
}

QString RobertWindowItem::caption() const
{
    return tr("R.O.B.E.R.T.");
}

void RobertWindowItem::setLoggedIn(bool loggedIn)
{
    loggedIn_ = loggedIn;
    updateVisibility();
}

void RobertWindowItem::clearFilters()
{
    manageRulesItem_ = nullptr;
    for (auto item : groups_) {
        removeItem(item);
    }
    groups_.clear();
}

void RobertWindowItem::setFilters(const QVector<api_responses::RobertFilter> &filters)
{
    loading_ = false;

    if (items().isEmpty()) {
        addItem(desc_);
    }

    groups_.clear();

    QList<CommonGraphics::BaseItem *> newItems;

    for(auto filter : filters) {
        PreferenceGroup *group = new PreferenceGroup(this);
        RobertItem *item = new RobertItem(group, filter);
        connect(item, &RobertItem::filterChanged, this, &RobertWindowItem::setRobertFilter);
        group->addItem(item);
        newItems << group;
        groups_ << group;
    }

    if (!manageRulesItem_) {
        PreferenceGroup *manageRulesGroup = new PreferenceGroup(this);
        manageRulesItem_ = new LinkItem(manageRulesGroup, LinkItem::LinkType::EXTERNAL_LINK);
        connect(manageRulesItem_, &LinkItem::clicked, this, &RobertWindowItem::onManageRobertRulesClick);
        manageRulesGroup->addItem(manageRulesItem_);
        addItem(manageRulesGroup);
        groups_ << manageRulesGroup;
    }

    setItems(newItems, 1, items().size() - 1);

    onLanguageChanged();
    updateVisibility();
}

void RobertWindowItem::updateScaling()
{
    BasePage::updateScaling();
    updatePositions();
    errorMessage_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    loginPrompt_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
}

void RobertWindowItem::updatePositions()
{
    int errorCenterX = static_cast<int>(boundingRect().width()/2 - errorMessage_->boundingRect().width()/2);
    errorMessage_->setPos(errorCenterX, kMessageOffsetY*G_SCALE);

    int promptCenterX = static_cast<int>(boundingRect().width()/2 - loginPrompt_->boundingRect().width()/2);
    loginPrompt_->setPos(promptCenterX, kMessageOffsetY*G_SCALE);

    int loginCenterX = static_cast<int>(boundingRect().width()/2 - loginButton_->boundingRect().width()/2);
    loginButton_->setPos(loginCenterX, (kMessageOffsetY + PREFERENCES_MARGIN_Y)*G_SCALE + loginPrompt_->boundingRect().height());

    int loadingCenterX = static_cast<int>(boundingRect().width()/2 - kLoadingIconSize*G_SCALE/2);
    loadingIcon_->setPos(loadingCenterX, kLoadingIconOffsetY*G_SCALE);
}

void RobertWindowItem::setError(bool isError)
{
    loading_ = false;
    isError_ = isError;
    updateVisibility();
}

void RobertWindowItem::setLoading(bool loading)
{
    loading_ = loading;
    updateVisibility();
}

void RobertWindowItem::updateVisibility()
{
    if (!loggedIn_) {
        desc_->setVisible(loggedIn_);
        for (auto group : groups_) {
            group->setVisible(loggedIn_);
        }
        errorMessage_->setVisible(false);
        loginPrompt_->setVisible(!loggedIn_);
        loginButton_->setVisible(!loggedIn_);
        loadingIcon_->setVisible(false);
        loadingIcon_->stop();
    } else {
        loginPrompt_->setVisible(false);
        loginButton_->setVisible(false);

        if (!isError_ && groups_.isEmpty()) {
            desc_->setVisible(false);
            errorMessage_->setVisible(false);
            loadingIcon_->setVisible(true);
            if (loading_) {
                loadingIcon_->start();
            } else {
                loadingIcon_->stop();
            }
        } else {
            desc_->setVisible(!isError_);
            errorMessage_->setVisible(isError_);
            loadingIcon_->setVisible(false);
            loadingIcon_->stop();
            for (auto group : groups_) {
                group->setVisible(!isError_);
            }
        }
    }
}

void RobertWindowItem::onLanguageChanged()
{
    desc_->setDescription(tr("R.O.B.E.R.T. is a customizable server-side domain and IP blocking tool. Select the block lists you wish to apply on all your devices by toggling the switch."),
                          QString("https://%1/features/robert").arg(HardcodedSettings::instance().windscribeServerUrl()));
    errorMessage_->setPlainText(tr("Could not retrieve R.O.B.E.R.T. preferences from server. Try again later."));
    loginPrompt_->setPlainText(tr("Login to view or change R.O.B.E.R.T preferences"));
    loginButton_->setText(tr("Login"));
    if (manageRulesItem_) {
        manageRulesItem_->setTitle(tr("Manage Custom Rules"));
    }
}

void RobertWindowItem::onManageRobertRulesClick()
{
    manageRulesItem_->setInProgress(true);
    emit manageRobertRulesClick();
}

void RobertWindowItem::setWebSessionCompleted()
{
    if (manageRulesItem_) {
        manageRulesItem_->setInProgress(false);
    }
}

} // namespace PreferencesWindow
