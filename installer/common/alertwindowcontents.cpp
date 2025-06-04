#include "alertwindowcontents.h"

#include <QApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QScreen>
#include <QWidget>

#include "svgresources.h"
#include "themecontroller.h"

AlertWindowContents::AlertWindowContents(QWidget *parent)
    : QWidget(parent), primaryButtonColor_(ThemeController::instance().primaryButtonColor()), titleSize_(18), descSize_(13)
{
    icon_ = new QLabel(this);

    title_ = new QLabel(this);
    title_->setWordWrap(true);
    title_->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    desc_ = new QLabel(this);
    desc_->setWordWrap(true);
    desc_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    primaryButton_ = new QPushButton(this);
    primaryButton_->setCursor(Qt::PointingHandCursor);
    connect(primaryButton_, &QPushButton::clicked, this, &AlertWindowContents::primaryButtonClicked);

    secondaryButton_ = new QPushButton(this);
    secondaryButton_->setCursor(Qt::PointingHandCursor);
    secondaryButton_->setVisible(false);
    connect(secondaryButton_, &QPushButton::clicked, this, &AlertWindowContents::escapeClicked);

    onThemeChanged();
    updateDimensions();
}

AlertWindowContents::~AlertWindowContents()
{
}

void AlertWindowContents::onThemeChanged()
{
    QString fontColor = ThemeController::instance().defaultFontColor().name();
    QString fontName = ThemeController::instance().defaultFontName();

    title_->setStyleSheet(QString("color: %1; font-family: %2; font-size: %3px; font-weight: 600;").arg(fontColor).arg(fontName).arg(titleSize_));
    desc_->setStyleSheet(QString("color: %1; font-family: %2; font-size: %3px;").arg(fontColor).arg(fontName).arg(descSize_));

    QString primaryButtonStyle;
    primaryButtonStyle += "QPushButton { background-color: %1; color: %2; border: none; border-radius: 17px; font-family: %3; font-size: 14px; }";
    primaryButtonStyle += "QPushButton:hover { background-color: %4; color: %5 }";
    primaryButtonStyle += "QPushButton:pressed { background-color: %4; color: %5 }";
    primaryButton_->setStyleSheet(primaryButtonStyle
        .arg(primaryButtonColor_.name(QColor::HexArgb))
        .arg(primaryButtonFontColor_.name(QColor::HexArgb))
        .arg(fontName)
        .arg(ThemeController::instance().primaryButtonHoverColor().name())
        .arg(ThemeController::instance().primaryButtonFontColor().name()));

    QString secondaryButtonStyle;
    secondaryButtonStyle += "QPushButton { color: %1; border: none; font-family: %2; font-size: 14px; font-weight: 600; }";
    secondaryButtonStyle += "QPushButton:hover { color: %3; }";
    secondaryButtonStyle += "QPushButton:pressed { color: %3; }";
    secondaryButton_->setStyleSheet(secondaryButtonStyle
        .arg(ThemeController::instance().secondaryButtonColor().name())
        .arg(fontName)
        .arg(ThemeController::instance().secondaryButtonHoverColor().name()));

    update();
}

void AlertWindowContents::updateDimensions()
{
    qreal pmWidth = icon_->pixmap().width()/icon_->pixmap().devicePixelRatio();
    qreal pmHeight = icon_->pixmap().height()/icon_->pixmap().devicePixelRatio();
    icon_->setGeometry((314 - pmWidth)/2, 48, pmWidth, pmHeight);

    QFont titleFont(ThemeController::instance().defaultFontName());
    titleFont.setWeight(QFont::DemiBold);
    titleFont.setPixelSize(titleSize_);
    QFontMetrics titleFm(titleFont);
    title_->setFont(titleFont);
    title_->setGeometry(0, 76 + icon_->height(), 314, titleFm.boundingRect(0, 0, 314, height(), Qt::TextWordWrap, title_->text()).height());

    QFont descFont(ThemeController::instance().defaultFontName());
    descFont.setPixelSize(descSize_);
    QFontMetrics fmDesc(descFont);
    desc_->setGeometry(0, 92 + icon_->height() + title_->height(), 314, fmDesc.boundingRect(0, 0, 314, height(), Qt::TextWordWrap, desc_->text()).height());

    if (!primaryButton_->text().isEmpty()) {
        QFontMetrics fm(primaryButton_->font());
        int buttonWidth = fm.horizontalAdvance(primaryButton_->text()) + 32;
        primaryButton_->setGeometry(157 - buttonWidth/2, 112 + icon_->height() + title_->height() + desc_->height(), buttonWidth, 35);
        secondaryButton_->move((314 - secondaryButton_->width())/2, 128 + icon_->height() + title_->height() + desc_->height() + secondaryButton_->height() + 16);
    } else if (!secondaryButton_->text().isEmpty()) {
        secondaryButton_->move((314 - secondaryButton_->width())/2, 112 + icon_->height() + title_->height() + desc_->height() + 16);
    }

    int height =
        48 + // height above icon
        icon_->height() +
        20 + // space between icon and title
        title_->height() +
        16 + // space between title and desc
        desc_->height() +
        30; // space between desc and button

    if (!primaryButton_->text().isEmpty()) {
        height += primaryButton_->height() + 16;
    }

    if (!secondaryButton_->text().isEmpty()) {
        height += secondaryButton_->height() + 16;
    }

    setFixedHeight(height);
    setFixedWidth(350);
    onThemeChanged();
    emit sizeChanged();
}

void AlertWindowContents::setIcon(const QString &path)
{
    icon_->setPixmap(SvgResources::instance().pixmap(path));
    updateDimensions();
}

void AlertWindowContents::setTitle(const QString &title)
{
    title_->setText(title);
    updateDimensions();
}

void AlertWindowContents::setTitleSize(int px)
{
    titleSize_ = px;
    updateDimensions();
}

void AlertWindowContents::setDescription(const QString &desc)
{
    desc_->setText(desc);
    updateDimensions();
}

void AlertWindowContents::setPrimaryButton(const QString &text)
{
    primaryButton_->setText(text);
    if (text.isEmpty()) {
        primaryButton_->hide();
    } else {
        primaryButton_->show();
    }
    updateDimensions();
}

void AlertWindowContents::setPrimaryButtonColor(const QColor &color)
{
    primaryButtonColor_ = color;
    onThemeChanged();
}

void AlertWindowContents::setPrimaryButtonFontColor(const QColor &color)
{
    primaryButtonFontColor_ = color;
    onThemeChanged();
}

void AlertWindowContents::setSecondaryButton(const QString &text)
{
    secondaryButton_->setText(text);
    if (text.isEmpty()) {
        secondaryButton_->hide();
    } else {
        secondaryButton_->show();
    }
    updateDimensions();
}

void AlertWindowContents::setDescriptionSize(int px)
{
    descSize_ = px;
    updateDimensions();
}
