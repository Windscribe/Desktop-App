#include "upgradebanner.h"

#include <QLocale>
#include <QPainter>
#include <QEnterEvent>
#include <QMouseEvent>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "utils/log/categories.h"

UpgradeBanner::UpgradeBanner(QWidget *parent)
    : QWidget(parent), bytesUsed_(-1), bytesMax_(-1), opacity_(OPACITY_HALF)
{
    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &UpgradeBanner::onOpacityChanged);
    opacityAnimation_.setDuration(ANIMATION_SPEED_FAST);
}

void UpgradeBanner::setDataRemaining(qint64 bytesUsed, qint64 bytesMax)
{
    bytesUsed_ = bytesUsed;
    bytesMax_ = bytesMax;
    update();
}

void UpgradeBanner::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // draw background
    QPen pen = p.pen();
    pen.setColor(QColor(255, 255, 255, 26));
    pen.setWidth(2);
    p.setPen(pen);
    p.setBrush(QColor(9, 15, 25));
    p.drawRoundedRect(QRect(0, 0, width(), height()), 8, 8);

    // data left circle background
    pen.setColor(QColor(255, 255, 255, 51));
    pen.setWidth(3);
    p.setPen(pen);
    p.drawArc(QRect(12*G_SCALE, 12*G_SCALE, 40*G_SCALE, 40*G_SCALE), 0, 360*16);

    // data left circle
    pen.setColor(getStatusColor());
    p.setPen(pen);
    double pct = 1 - (double)bytesUsed_/(double)bytesMax_;
    p.drawArc(QRect(12*G_SCALE, 12*G_SCALE, 40*G_SCALE, 40*G_SCALE), 180*16, -pct*360*16);

    // text inside circle
    QFont font = FontManager::instance().getFont(9, QFont::Medium);
    p.setFont(font);
    QLocale locale(LanguageController::instance().getLanguage());
    QString text = locale.formattedDataSize(bytesMax_ - bytesUsed_, 2, QLocale::DataSizeTraditionalFormat);
    QString word1 = text.section(" ", 0, 0, QString::SectionSkipEmpty);
    QString word2 = text.section(" ", 1, 1, QString::SectionSkipEmpty);

    p.drawText(12*G_SCALE, 23*G_SCALE, 40*G_SCALE, 40*G_SCALE, Qt::AlignHCenter, word1);
    p.drawText(12*G_SCALE, 32*G_SCALE, 40*G_SCALE, 40*G_SCALE, Qt::AlignHCenter, word2);

    p.setOpacity(opacity_);

    QString unlock = tr("Unlock full access to Windscribe");
    font = FontManager::instance().getFont(15, QFont::Medium);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(68*G_SCALE, 12*G_SCALE, geometry().width() - 70*G_SCALE, 20*G_SCALE, Qt::AlignLeft, unlock);

    QString goPro = tr("Go Pro for unlimited everything");
    font = FontManager::instance().getFont(12, QFont::Normal);
    p.setFont(font);
    p.setPen(QColor(59, 255, 239, 179));
    p.drawText(68*G_SCALE, 36*G_SCALE, geometry().width() - 70*G_SCALE, 16*G_SCALE, Qt::AlignLeft, goPro);

    QSharedPointer<IndependentPixmap> pm = ImageResourcesSvg::instance().getIndependentPixmap("locations/ARROW_RIGHT");
    pm->draw(302*G_SCALE, 24*G_SCALE, 16*G_SCALE, 16*G_SCALE, &p);
}

void UpgradeBanner::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    startAnAnimation<double>(opacityAnimation_, opacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    setCursor(Qt::PointingHandCursor);
}

void UpgradeBanner::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    startAnAnimation<double>(opacityAnimation_, opacity_, OPACITY_HALF, ANIMATION_SPEED_FAST);
    setCursor(Qt::ArrowCursor);
}

void UpgradeBanner::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void UpgradeBanner::onOpacityChanged(const QVariant &value)
{
    opacity_ = value.toDouble();
    update();
}

QColor UpgradeBanner::getStatusColor() const
{
    qint64 bytesLeft = bytesMax_ - bytesUsed_;

    if (bytesMax_ == -1 || bytesLeft > TEN_GB_IN_BYTES/2) { // 5+ GB
        return FontManager::instance().getSeaGreenColor();
    } else if (bytesLeft > TEN_GB_IN_BYTES/10) { // 1-4 GB
        return FontManager::instance().getBrightYellowColor();
    } else { // < 1GB
        return FontManager::instance().getErrorRedColor();
    }
}
