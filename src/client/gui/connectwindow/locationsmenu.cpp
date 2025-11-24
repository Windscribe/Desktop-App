#include "locationsmenu.h"

#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "backend/persistentstate.h"

namespace ConnectWindow {

LocationsMenu::LocationsMenu(ScalableGraphicsObject *parent)
    : ScalableGraphicsObject(parent), curSearchPos_(140), isExternalConfigMode_(false)
{
    allLocations_ = new IconButton(32, 32, "locations/ALL_LOCATION_ICON_DESELECTED", "", this);
    connect(allLocations_, &IconButton::clicked, this, &LocationsMenu::onAllLocationsClicked);
    favoriteLocations_ = new IconButton(32, 32, "locations/FAV_ICON_DESELECTED", "", this);
    connect(favoriteLocations_, &IconButton::clicked, this, &LocationsMenu::onFavoriteLocationsClicked);
    configuredLocations_ = new IconButton(32, 32, "locations/CONFIG_ICON_DESELECTED", "", this);
    connect(configuredLocations_, &IconButton::clicked, this, &LocationsMenu::onConfiguredLocationsClicked);
    staticIpsLocations_ = new IconButton(32, 32, "locations/STATIC_IP_ICON_DESELECTED", "", this);
    connect(staticIpsLocations_, &IconButton::clicked, this, &LocationsMenu::onStaticIpsLocationsClicked);
    searchLocations_ = new IconButton(32, 32, "locations/SEARCH_ICON_DESELECTED", "", this);
    connect(searchLocations_, &IconButton::clicked, this, &LocationsMenu::onSearchLocationsClicked);

    searchLineEdit_ = new CommonWidgets::CustomMenuLineEdit();
    searchLineEdit_->setFont(FontManager::instance().getFont(14,  QFont::Normal));
    searchLineEdit_->setStyleSheet("background: transparent; color: rgb(255, 255, 255)");
    searchLineEdit_->setFrame(false);
    searchLineEdit_->installEventFilter(this);
    connect(searchLineEdit_, &CommonWidgets::CustomMenuLineEdit::textChanged, this, &LocationsMenu::onSearchLineEditTextChanged);
    searchLineEdit_->hide();

    connect(&searchAnimation_, &QVariantAnimation::valueChanged, this, &LocationsMenu::onSearchAnimationValueChanged);
    connect(&searchAnimation_, &QVariantAnimation::finished, this, &LocationsMenu::onSearchAnimationFinished);

    searchTypingDelayTimer_.setInterval(100);
    connect(&searchTypingDelayTimer_, &QTimer::timeout, this, &LocationsMenu::onSearchTypingDelayTimerTimeout);

    searchCancel_ = new IconButton(32, 32, "locations/SEARCH_CANCEL_ICON", "", this);
    connect(searchCancel_, &IconButton::clicked, this, &LocationsMenu::onSearchCancelClicked);
    searchCancel_->hide();

    lineEditProxy_ = new QGraphicsProxyWidget(this);
    lineEditProxy_->setWidget(searchLineEdit_);
    setFocusProxy(lineEditProxy_);
    lineEditProxy_->hide();

    updatePositions();

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &LocationsMenu::onLanguageChanged);
    onLanguageChanged();

    // Do this when main thread returns to the event loop.  We can't do this now because these events trigger signals which are not connected by parent yet.
    QTimer::singleShot(0, this, [this]() {
        switch(PersistentState::instance().lastLocationTab()) {
        case LOCATION_TAB_CONFIGURED_LOCATIONS:
            onConfiguredLocationsClicked();
            break;
        case LOCATION_TAB_STATIC_IPS_LOCATIONS:
            onStaticIpsLocationsClicked();
            break;
        case LOCATION_TAB_FAVORITE_LOCATIONS:
            onFavoriteLocationsClicked();
            break;
        default:
            onAllLocationsClicked();
            break;
        }
    });
}

QRectF LocationsMenu::boundingRect() const
{
    return QRectF(0, 0, 156*G_SCALE, 32*G_SCALE);
}

void LocationsMenu::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void LocationsMenu::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void LocationsMenu::updatePositions()
{
    if (!isExternalConfigMode_) {
        allLocations_->setPos(0*G_SCALE, 0);
        favoriteLocations_->setPos(35*G_SCALE, 0);
        staticIpsLocations_->setPos(70*G_SCALE, 0);
        configuredLocations_->setPos(105*G_SCALE, 0);
        searchLocations_->setPos(curSearchPos_*G_SCALE, 0);

        searchLineEdit_->setGeometry(32*G_SCALE, 4*G_SCALE, 108*G_SCALE, 24*G_SCALE);
        searchCancel_->setPos(140*G_SCALE, 0);
    } else {
        configuredLocations_->setPos(140*G_SCALE, 0);
    }
}

void LocationsMenu::onAllLocationsClicked()
{
    updateSelectedTab(allLocations_);
    prevTab_ = curTab_;
    curTab_ = LOCATION_TAB_ALL_LOCATIONS;
    PersistentState::instance().setLastLocationTab(curTab_);
    emit locationTabClicked(LOCATION_TAB_ALL_LOCATIONS);
}

void LocationsMenu::onConfiguredLocationsClicked()
{
    updateSelectedTab(configuredLocations_);
    prevTab_ = curTab_;
    curTab_ = LOCATION_TAB_CONFIGURED_LOCATIONS;
    PersistentState::instance().setLastLocationTab(curTab_);
    emit locationTabClicked(LOCATION_TAB_CONFIGURED_LOCATIONS);
}

void LocationsMenu::onStaticIpsLocationsClicked()
{
    updateSelectedTab(staticIpsLocations_);
    prevTab_ = curTab_;
    curTab_ = LOCATION_TAB_STATIC_IPS_LOCATIONS;
    PersistentState::instance().setLastLocationTab(curTab_);
    emit locationTabClicked(LOCATION_TAB_STATIC_IPS_LOCATIONS);
}

void LocationsMenu::onFavoriteLocationsClicked()
{
    updateSelectedTab(favoriteLocations_);
    prevTab_ = curTab_;
    curTab_ = LOCATION_TAB_FAVORITE_LOCATIONS;
    PersistentState::instance().setLastLocationTab(curTab_);
    emit locationTabClicked(LOCATION_TAB_FAVORITE_LOCATIONS);
}

void LocationsMenu::onSearchLocationsClicked()
{
    updateSelectedTab(searchLocations_);
    prevTab_ = curTab_;
    curTab_ = LOCATION_TAB_SEARCH_LOCATIONS;
    showSearch();
    emit locationTabClicked(LOCATION_TAB_SEARCH_LOCATIONS);
}

void LocationsMenu::updateSelectedTab(IconButton *selectedButton)
{
    static const QString prefixes[] = { "locations/ALL_LOCATION_ICON", "locations/CONFIG_ICON", "locations/STATIC_IP_ICON", "locations/FAV_ICON", "locations/SEARCH_ICON" };
    IconButton *buttons[] = { allLocations_, configuredLocations_, staticIpsLocations_, favoriteLocations_, searchLocations_ };

    for (int i = 0; i < 5; i++) {
        IconButton *button = buttons[i];
        if (button == selectedButton) {
            button->setIcon(prefixes[i] + "_SELECTED", false);
            button->setUnhoverOpacity(OPACITY_FULL);
            button->hover();
        } else {
            button->setIcon(prefixes[i] + "_DESELECTED", false);
            button->setUnhoverOpacity(OPACITY_HALF);
            button->unhover();
        }
    }
}

void LocationsMenu::onLanguageChanged()
{
    allLocations_->setTooltip(tr("All"));
    configuredLocations_->setTooltip(tr("Custom configs"));
    staticIpsLocations_->setTooltip(tr("Static IPs"));
    favoriteLocations_->setTooltip(tr("Favourites"));
    searchLocations_->setTooltip(tr("Search"));
}

bool LocationsMenu::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && object == searchLineEdit_) {
        return handleKeyPressEvent(static_cast<QKeyEvent*>(event));
    }
    return ScalableGraphicsObject::eventFilter(object, event);
}

void LocationsMenu::onSearchLineEditTextChanged(const QString &text)
{
    filterText_ = text;
    searchTypingDelayTimer_.start();
}

void LocationsMenu::onSearchCancelClicked()
{
    searchLineEdit_->setText("");
    hideSearch();
    switch(prevTab_) {
        case LOCATION_TAB_CONFIGURED_LOCATIONS:
            onConfiguredLocationsClicked();
            break;
        case LOCATION_TAB_STATIC_IPS_LOCATIONS:
            onStaticIpsLocationsClicked();
            break;
        case LOCATION_TAB_FAVORITE_LOCATIONS:
            onFavoriteLocationsClicked();
            break;
        default:
        case LOCATION_TAB_ALL_LOCATIONS:
            onAllLocationsClicked();
            break;
    }
}

void LocationsMenu::showSearch()
{
    allLocations_->hide();
    configuredLocations_->hide();
    staticIpsLocations_->hide();
    favoriteLocations_->hide();

    // Line edit & cancel button are shown when animation is finished
    startAnAnimation(searchAnimation_, curSearchPos_, 0, ANIMATION_SPEED_FAST);
}

void LocationsMenu::hideSearch()
{
    searchLineEdit_->hide();
    searchCancel_->hide();
    updatePositions();

    // Menu buttons are shown when animation is finished
    startAnAnimation(searchAnimation_, curSearchPos_, 140, ANIMATION_SPEED_FAST);
}

void LocationsMenu::onSearchAnimationValueChanged(const QVariant &value)
{
    curSearchPos_ = value.toInt();
    searchLocations_->setPos(curSearchPos_*G_SCALE, 0);
}

void LocationsMenu::onSearchAnimationFinished()
{
    if (curSearchPos_ == 0) {
        searchCancel_->show();
        searchLineEdit_->show();
        searchLineEdit_->setFocus();
        updatePositions();
    } else {
        allLocations_->show();
        configuredLocations_->show();
        staticIpsLocations_->show();
        favoriteLocations_->show();
        updatePositions();
    }
}

void LocationsMenu::onSearchTypingDelayTimerTimeout()
{
    emit searchFilterChanged(filterText_);
    searchTypingDelayTimer_.stop();
}

bool LocationsMenu::handleKeyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (curTab_ == LOCATION_TAB_SEARCH_LOCATIONS) {
            if (searchLineEdit_->text() == "") { // Search is open
                onSearchCancelClicked();
            } else if (curSearchPos_ == 0) {
                searchLineEdit_->setText("");
            }
            return true;
        } else {
            // Parent will handle collapsing locations menu
            return false;
        }
    } else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        emit locationsKeyPressed(event);
        return true;
    } else if (event->key() == Qt::Key_PageDown || event->key() == Qt::Key_PageUp || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        emit locationsKeyPressed(event);
        return true;
    } else if (event->key() == Qt::Key_Left) {
        return handleLeftKey();
    } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Tab) {
        return handleRightKey();
    } else if (event->key() == Qt::Key_Home) {
        searchLineEdit_->home(event->modifiers() & Qt::ShiftModifier);
        return true;
    } else if (event->key() == Qt::Key_End) {
        searchLineEdit_->end(event->modifiers() & Qt::ShiftModifier);
        return true;
    } else {
        if (curTab_ != LOCATION_TAB_SEARCH_LOCATIONS) {
            // If search tab is not open, and the user presses space, collapse locations.
            // For any other printable character, open the search tab and append the text.
            if (event->key() != Qt::Key_Space && event->key() != Qt::Key_Meta && QChar(event->key()).isPrint() &&
                (!(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::ShiftModifier))))
            {
                onSearchLocationsClicked();
                searchLineEdit_->setFocus();
                return searchLineEdit_->event(event);
            }
        } else {
            // Already on search tab, append the text, if modifier keys are not pressed
            searchLineEdit_->setFocus();
            return searchLineEdit_->event(event);
        }
    }
    return false;
}

bool LocationsMenu::handleLeftKey()
{
    if (curTab_ == LOCATION_TAB_SEARCH_LOCATIONS || curTab_ == LOCATION_TAB_ALL_LOCATIONS) {
        return false;
    }

    if (curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS) {
        onStaticIpsLocationsClicked();
    } else if (curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS) {
        onFavoriteLocationsClicked();
    } else if (curTab_ == LOCATION_TAB_FAVORITE_LOCATIONS) {
        onAllLocationsClicked();
    }
    return true;
}

bool LocationsMenu::handleRightKey()
{
    if (curTab_ == LOCATION_TAB_SEARCH_LOCATIONS) {
        return false;
    }

    if (curTab_ == LOCATION_TAB_ALL_LOCATIONS) {
        onFavoriteLocationsClicked();
    } else if (curTab_ == LOCATION_TAB_CONFIGURED_LOCATIONS) {
        onSearchLocationsClicked();
    } else if (curTab_ == LOCATION_TAB_STATIC_IPS_LOCATIONS) {
        onConfiguredLocationsClicked();
    } else if (curTab_ == LOCATION_TAB_FAVORITE_LOCATIONS) {
        onStaticIpsLocationsClicked();
    }
    return true;
}

void LocationsMenu::dismiss()
{
    if (curTab_ == LOCATION_TAB_SEARCH_LOCATIONS) {
        onSearchCancelClicked();
    }
}

void LocationsMenu::setIsExternalConfigMode(bool isExternalConfigMode)
{
    isExternalConfigMode_ = isExternalConfigMode;

    if (isExternalConfigMode) {
        allLocations_->hide();
        staticIpsLocations_->hide();
        favoriteLocations_->hide();
        searchLocations_->hide();
        searchLineEdit_->hide();
        searchCancel_->hide();
        onConfiguredLocationsClicked();
        updatePositions();
    } else {
        allLocations_->show();
        staticIpsLocations_->show();
        favoriteLocations_->show();
        searchLocations_->show();
        updatePositions();
    }
}

} // namespace ConnectWindow
