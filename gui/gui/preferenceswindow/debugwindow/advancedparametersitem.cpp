#include "advancedparametersitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

AdvancedParametersItem::AdvancedParametersItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
  , saved_(false)
{
    QString ss = "QPlainTextEdit { background: rgb(2,13,28); color: rgb(255, 255, 255) }";
    //QString ss = "QPlainTextEdit { background: rgb(100,100,100); color: rgb(255, 255, 255) }";

    textEdit_ = new CustomTextEditWidget();
    textEdit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEdit_->setContextMenuPolicy(Qt::NoContextMenu);
    textEdit_->setAttribute(Qt::WA_OpaquePaintEvent, false);
    textEdit_->setPlaceholderText(tr("Write your parameters here"));
    textEdit_->setFont(*FontManager::instance().getFont(12, false));
    textEdit_->setStyleSheet(ss);
    textEdit_->setBackgroundVisible(false);
    textEdit_->setFrameStyle(QFrame::NoFrame);
    textEdit_->setFixedWidth(static_cast<int>(boundingRect().width())- 70);
    textEdit_->verticalScrollBar()->setEnabled(false);
    textEdit_->setStepSize(textEdit_->lineHeight());

    textEdit_->setPlainText("Some plain text for display\n"  "more text\n"  "even more\n" "more\n"
                            "a\n" "b\n"   "d\n"   "e\n"    "f\n"       "g\n"            "h\n"        "i\n"
                            "j\n"    "k\n"    "l\n"   "m\n"      "n\n"      "o\n"    "p\n"        "q\n"
                            "r\n"   "s\n"  "t\n" "u\n"  "v\n"    "w\n"   "x\n" "y\n"    "z\n"
                            );

    scrollArea_ = new ScrollAreaWidget(boundingRect().width()- 56, 300 - RECT_MARGIN_HEIGHT);
    scrollArea_->move(TEXT_MARGIN_WIDTH, TEXT_MARGIN_HEIGHT);
    scrollArea_->setCentralWidget(textEdit_);
    scrollArea_->setStyleSheet("QWidget { background: rgb(2,13,28) }");
    scrollArea_->setLayoutConstraint(QLayout::SetFixedSize); // need for proper clipping
    scrollArea_->setMaximumHeight(scrollArea_->sizeHint().height()); // clips central widget
    scrollArea_->updateSizing();

    connect(textEdit_, SIGNAL(textChanged()), SLOT(onTextEditTextChanged()));

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(scrollArea_);

    clearButton_ = new CommonGraphics::BubbleButtonDark(this, 69, 24, 12, 20);
    clearButton_->setText(tr("Clear"));
    clearButton_->setFont(FontDescr(12,false));
    connect(clearButton_, SIGNAL(clicked()), SLOT(onClearClicked()));

    saveButton_ = new CommonGraphics::BubbleButtonBright(this, 69, 24, 12, 20);
    saveButton_->setText(tr("Save"));
    saveButton_->setFont(FontDescr(12,false));
    connect(saveButton_, SIGNAL(clicked()), SLOT(onSaveClicked()));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    connect(this, SIGNAL(heightChanged(int)), SLOT(onHeightChanged(int)));

    saveTextColorAnimation_.setTargetObject(this);
    saveTextColorAnimation_.setParent(this);
    saveTextColorAnimation_.setStartValue(FontManager::instance().getMidnightColor());
    saveTextColorAnimation_.setEndValue(FontManager::instance().getSeaGreenColor());
    saveTextColorAnimation_.setDuration(ANIMATION_SPEED_FAST);
    connect(&saveTextColorAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onSaveTextColorChanged(QVariant)));
    connect (&saveFillOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onSaveFillOpacityChanged(QVariant)));
}

void AdvancedParametersItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setPen(Qt::white);
    painter->setBrush(Qt::transparent);
    painter->setOpacity(0.15 * initOpacity);

    const int kMarginWidth = RECT_MARGIN_WIDTH * G_SCALE;
    const int kMarginHeight = RECT_MARGIN_HEIGHT * G_SCALE;
    painter->drawRoundedRect(QRect(kMarginWidth, kMarginHeight,
                                   boundingRect().width() - kMarginWidth * 2,
                                   height_ * G_SCALE - clearButton_->boundingRect().height()
                                                     - kMarginHeight * 2), 5, 5);
}

void AdvancedParametersItem::setAdvancedParameters(const QString &advParams)
{
    textEdit_->setPlainText(advParams);
}

void AdvancedParametersItem::onLanguageChanged()
{
    textEdit_->setPlaceholderText(tr("Write your parameters here"));
    clearButton_->setText(tr("Clear"));
    saveButton_->setText(tr("Save"));

    updateWidgetPositions();
}

void AdvancedParametersItem::onClearClicked()
{
    textEdit_->undoableClear();
}

void AdvancedParametersItem::onSaveClicked()
{
    emit advancedParametersChanged(textEdit_->toPlainText());
    saveButton_->setText(tr("Saved!"));

    saveButton_->setClickable(false);

    startAnAnimation<double>(saveFillOpacityAnimation_, saveButton_->curFillOpacity(), 0.2, ANIMATION_SPEED_FAST);

    saveTextColorAnimation_.setDirection(QVariantAnimation::Forward);
    if (saveTextColorAnimation_.state() != QVariantAnimation::Running)
    {
        saveTextColorAnimation_.start();
    }

    saved_ = true;
}

void AdvancedParametersItem::onTextEditTextChanged()
{
    if (saved_)
    {
        saved_ = false;
        saveButton_->setText(tr("Save"));

        saveButton_->setClickable(true);

        startAnAnimation<double>(saveFillOpacityAnimation_, saveButton_->curFillOpacity(), OPACITY_FULL, ANIMATION_SPEED_FAST);

        saveTextColorAnimation_.setDirection(QVariantAnimation::Backward);
        if (saveTextColorAnimation_.state() != QVariantAnimation::Running)
        {
            saveTextColorAnimation_.start();
        }
    }

    scrollArea_->updateSizing();

    updateWidgetPositions();
}

void AdvancedParametersItem::updateWidgetPositions()
{
    const int kMarginWidth = RECT_MARGIN_WIDTH * G_SCALE;
    const int kMarginHeight = RECT_MARGIN_HEIGHT * G_SCALE;

    clearButton_->setPos(kMarginWidth, height_ * G_SCALE - kMarginHeight);
    saveButton_->setPos(boundingRect().width() - saveButton_->boundingRect().width() - kMarginWidth,
                        height_ * G_SCALE - kMarginHeight);

    scrollArea_->setHeight(height_ - 90);
    const auto scrollSize = scrollArea_->sizeHint();

    // The order of the following calls is important, do not touch.
    scrollArea_->setMinimumWidth(scrollSize.width());
    scrollArea_->setMaximumSize(scrollSize);
    proxyWidget_->setMinimumSize(scrollArea_->minimumSize());
    proxyWidget_->update();
}

void AdvancedParametersItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    textEdit_->setFont(*FontManager::instance().getFont(12, false));
    textEdit_->setFixedWidth(boundingRect().width() - 70 * G_SCALE);
    textEdit_->updateScaling();

    scrollArea_->move(TEXT_MARGIN_WIDTH * G_SCALE, TEXT_MARGIN_HEIGHT * G_SCALE);
    updateWidgetPositions();
    scrollArea_->updateSizing();
}

void AdvancedParametersItem::onHeightChanged(int newHeight)
{
    Q_UNUSED(newHeight);

    updateWidgetPositions();
}

void AdvancedParametersItem::onSaveTextColorChanged(const QVariant &value)
{
    saveButton_->setTextColor(value.value<QColor>());
}

void AdvancedParametersItem::onSaveFillOpacityChanged(const QVariant &value)
{
    saveButton_->setFillOpacity(value.toDouble());
}

} // namespace PreferencesWindow
