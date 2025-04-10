#include "resizablewindow.h"

#include <QPainter>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QCoreApplication>

#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

ResizableWindow::ResizableWindow(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject(parent), preferences_(preferences), curScale_(G_SCALE)
{
    setFlags(QGraphicsObject::ItemIsFocusable);
    installEventFilter(this);

    minHeight_ = WINDOW_HEIGHT;
    maxHeight_ = INT_MAX;
    curHeight_ = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? minHeight_ - kVanGoghOffset : minHeight_;

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    backgroundBase_ = "background/WIN_TOP_BG";
    backgroundHeader_ = "background/WIN_HEADER_BG_OVERLAY";
    roundedFooter_ = false;
#else
    backgroundBase_ = "background/MAC_TOP_BG";
    backgroundHeader_ = "background/MAC_HEADER_BG_OVERLAY";
    roundedFooter_ = true;
#endif
    footerColor_ = FontManager::instance().getCharcoalColor();

    connect(preferences, &Preferences::appSkinChanged, this, &ResizableWindow::onAppSkinChanged);

    escapeButton_ = new CommonGraphics::EscapeButton(this);
    connect(escapeButton_, &CommonGraphics::EscapeButton::clicked, this, &ResizableWindow::escape);

    backArrowButton_ = new IconButton(32, 32, "BACK_ARROW", "", this);
    connect(backArrowButton_, &IconButton::clicked, this, &ResizableWindow::onBackArrowButtonClicked);

    bottomResizeItem_ = new CommonGraphics::ResizeBar(this);
    connect(bottomResizeItem_, &CommonGraphics::ResizeBar::resizeStarted, this, &ResizableWindow::onResizeStarted);
    connect(bottomResizeItem_, &CommonGraphics::ResizeBar::resizeChange, this, &ResizableWindow::onResizeChange);
    connect(bottomResizeItem_, &CommonGraphics::ResizeBar::resizeFinished, this, &ResizableWindow::onResizeFinished);

    scrollAreaItem_ = new CommonGraphics::ScrollArea(this, curHeight_ - 102*G_SCALE, WINDOW_WIDTH);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ResizableWindow::onLanguageChanged);

    updatePositions();
    // trigger app skin change in case we start in van gogh mode
    onAppSkinChanged(preferences_->appSkin());
}

ResizableWindow::~ResizableWindow()
{
}

QRectF ResizableWindow::boundingRect() const
{
    return QRectF(0, 0, WINDOW_WIDTH*G_SCALE, curHeight_);
}

void ResizableWindow::setMinimumHeight(int height)
{
    minHeight_ = height;
}

int ResizableWindow::minimumHeight()
{
    return preferences_->appSkin() == APP_SKIN_VAN_GOGH ? minHeight_ - kVanGoghOffset : minHeight_;
}

void ResizableWindow::setMaximumHeight(int height)
{
    if (height < minHeight_) {
        height = minHeight_;
    }
    maxHeight_ = height;

    if (curHeight_ > maxHeight_) {
        setHeight(maxHeight_*G_SCALE);
        emit sizeChanged(this);
    }
}

int ResizableWindow::maximumHeight()
{
    return preferences_->appSkin() == APP_SKIN_VAN_GOGH ? maxHeight_ - kVanGoghOffset : maxHeight_;
}

void ResizableWindow::setHeight(int height, bool ignoreMinimum)
{
    // Callers may set ignoreMinimum to true for transient animations.
    if (!ignoreMinimum && height < minimumHeight()*G_SCALE) {
        height = minimumHeight()*G_SCALE;
    } else if (height > maximumHeight()*G_SCALE) {
        height = maximumHeight()*G_SCALE;
    }

    prepareGeometryChange();
    curHeight_ = height;
    updatePositions();
    update();
}

void ResizableWindow::setScrollBarVisibility(bool on)
{
    scrollAreaItem_->setScrollBarVisibility(on);
}

void ResizableWindow::onResizeStarted()
{
    heightAtResizeStart_ = curHeight_;
}

void ResizableWindow::onResizeChange(int y)
{
    int min = minimumHeight();
    int max = maximumHeight();

    if ((heightAtResizeStart_ + y) >= min*G_SCALE && (heightAtResizeStart_ + y) <= max*G_SCALE) {
        prepareGeometryChange();
        curHeight_ = heightAtResizeStart_ + y;
        updatePositions();

        emit sizeChanged(this);
    }
}

void ResizableWindow::onResizeFinished()
{
    emit resizeFinished(this);
}

void ResizableWindow::onBackArrowButtonClicked()
{
    emit escape();
}

void ResizableWindow::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    curHeight_ = curHeight_*(G_SCALE/curScale_);
    curScale_ = G_SCALE;

    updatePositions();
}

void ResizableWindow::updatePositions()
{
    bottomResizeItem_->setPos(kBottomResizeOriginX*G_SCALE, curHeight_ - kBottomResizeOffsetY*G_SCALE);
    escapeButton_->onLanguageChanged();
    escapeButton_->setPos(WINDOW_WIDTH*G_SCALE - escapeButton_->boundingRect().width() - 16*G_SCALE, 16*G_SCALE);
    int heightOffset = 0;
    if (!bottomResizeItem_->isVisible()) {
        heightOffset = 9*G_SCALE;
    }

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        backArrowButton_->setPos(16*G_SCALE, 12*G_SCALE);
        scrollAreaItem_->setPos(0, 55*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 74*G_SCALE + heightOffset);
    } else {
        backArrowButton_->setPos(16*G_SCALE, 40*G_SCALE);
        scrollAreaItem_->setPos(0, 83*G_SCALE);
        scrollAreaItem_->setHeight(curHeight_ - 102*G_SCALE + heightOffset);
    }
}

void ResizableWindow::setBackButtonEnabled(bool b)
{
    backArrowButton_->setVisible(b);
}

void ResizableWindow::setResizeBarEnabled(bool b)
{
    bottomResizeItem_->setVisible(b);
}

void ResizableWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        // if inner item has focus let it handle the [Escape] event
        if (!scrollAreaItem_->hasItemWithFocus()) {
            onBackArrowButtonClicked();
        }
    } else {
        scrollAreaItem_->handleKeyPressEvent(event);
    }
}

QRectF ResizableWindow::getBottomResizeArea()
{
    return QRectF(0, curHeight_ - kBottomAreaHeight*G_SCALE, boundingRect().width(), kBottomAreaHeight*G_SCALE);
}

void ResizableWindow::onAppSkinChanged(APP_SKIN s)
{
    if (s == APP_SKIN_ALPHA) {
        escapeButton_->setTextPosition(CommonGraphics::EscapeButton::TEXT_POSITION_BOTTOM);
    } else if (s == APP_SKIN_VAN_GOGH) {
        escapeButton_->setTextPosition(CommonGraphics::EscapeButton::TEXT_POSITION_LEFT);
    }
    updatePositions();
    update();
    emit sizeChanged(this);
}

int ResizableWindow::scrollPos()
{
    if (!scrollAreaItem_->item()) {
        return 0;
    }
    return scrollAreaItem_->item()->y();
}

void ResizableWindow::setScrollPos(int pos)
{
    scrollAreaItem_->setScrollPos(pos);
}

void ResizableWindow::setScrollOffset(int offset)
{
    scrollAreaItem_->setScrollOffset(offset);
}

void ResizableWindow::onLanguageChanged()
{
    updatePositions();
}

void ResizableWindow::onWindowExpanded()
{
}

void ResizableWindow::onWindowCollapsed()
{
}
