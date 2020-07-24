#include "citynode.h"

#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"

namespace GuiLocations {

CityNode::CityNode(const LocationID &locationId, const QString &cityName1ForShow, const QString &cityName2ForShow, const QString &countryCode, PingTime timeMs, bool bShowPremiumStarOnly, bool isShowLatencyMs, const QString &staticIp, const QString &staticIpType, bool isFavorite, bool isDisabled) :
    locationId_(locationId),
    countryCode_(countryCode),
    timeMs_(timeMs, isShowLatencyMs), bShowPremiumStarOnly_(bShowPremiumStarOnly),
    staticIp_(staticIp), staticIpType_(staticIpType),
    isFavorite_(isFavorite),
    isCursorOverCaption1Text_(false),
    isDisabled_(isDisabled),
    isCursorOverConnectionMeter_(false)
{
    caption1FullText_ = cityName1ForShow;

    QFont *font = FontManager::instance().getFont(16, true);
    QString shortenedCaption1Text = CommonGraphics::truncateText(cityName1ForShow, *font, 175);

    caption1TextLayout_ = QSharedPointer<QTextLayout>(new QTextLayout(shortenedCaption1Text, *font));
    caption1TextLayout_->beginLayout();
    caption1TextLayout_->createLine();
    caption1TextLayout_->endLayout();
    caption1TextLayout_->setCacheEnabled(true);

    font = FontManager::instance().getFont(16, false);
    caption2TextLayout_ = QSharedPointer<QTextLayout>(new QTextLayout(cityName2ForShow, *font));
    caption2TextLayout_->beginLayout();
    caption2TextLayout_->createLine();
    caption2TextLayout_->endLayout();
    caption2TextLayout_->setCacheEnabled(true);

    if (!staticIp.isEmpty())
    {
        QFont *fontStaticIp = FontManager::instance().getFont(13, false);
        captionStaticIpLayout_ = QSharedPointer<QTextLayout>(new QTextLayout(staticIp, *fontStaticIp));
        captionStaticIpLayout_->beginLayout();
        captionStaticIpLayout_->createLine();
        captionStaticIpLayout_->endLayout();
        captionStaticIpLayout_->setCacheEnabled(true);
    }

    isCursorOverFavoriteIcon_ = false;
}

void CityNode::setConnectionSpeed(PingTime timeMs)
{
    timeMs_.setValue(timeMs);
}

bool CityNode::isShowPremiumStar() const
{
    return bShowPremiumStarOnly_;
}

PingTime CityNode::timeMs() const
{
    return timeMs_.getValue();
}

QTextLayout *CityNode::getCaption1TextLayout() const
{
    Q_ASSERT(!caption1TextLayout_.isNull());
    return caption1TextLayout_.data();
}

QTextLayout *CityNode::getCaption2TextLayout() const
{
    Q_ASSERT(!caption2TextLayout_.isNull());
    return caption2TextLayout_.data();
}

QTextLayout *CityNode::getStaticIpLayout() const
{
    Q_ASSERT(!captionStaticIpLayout_.isNull());
    return captionStaticIpLayout_.data();
}

QTextLayout *CityNode::getLatencyMsTextLayout()
{
    return timeMs_.getTextLayout();
}

LocationID CityNode::getLocationId() const
{
    return locationId_;
}

QString CityNode::getCountryCode() const
{
    return countryCode_;
}

QString CityNode::caption1FullText()
{
    return caption1FullText_;
}

bool CityNode::isCursorOverFavoriteIcon() const
{
    return isCursorOverFavoriteIcon_;
}

void CityNode::setCursorOverFavoriteIcon(bool b)
{
    isCursorOverFavoriteIcon_ = b;
}

bool CityNode::isCursorOverCaption1Text() const
{
    return isCursorOverCaption1Text_;
}

void CityNode::setCursorOverCaption1Text(bool b)
{
    isCursorOverCaption1Text_ = b;
}

bool CityNode::isCursorOverConnectionMeter() const
{
    return isCursorOverConnectionMeter_;
}

void CityNode::setCursorOverConnectionMeter(bool b)
{
    isCursorOverConnectionMeter_ = b;
}

bool CityNode::isFavorite() const
{
    return isFavorite_;
}

void CityNode::toggleIsFavorite()
{
    isFavorite_ = !isFavorite_;
}

bool CityNode::isDisabled() const
{
    return isDisabled_;
}

void CityNode::updateScaling()
{
    QFont *font = FontManager::instance().getFont(16, true);
    caption1TextLayout_->setFont(*font);
    caption1TextLayout_->beginLayout();
    caption1TextLayout_->createLine();
    caption1TextLayout_->endLayout();
    caption1TextLayout_->setCacheEnabled(true);

    QFont *font2 = FontManager::instance().getFont(16, false);
    caption2TextLayout_->setFont(*font2);
    caption2TextLayout_->beginLayout();
    caption2TextLayout_->createLine();
    caption2TextLayout_->endLayout();
    caption2TextLayout_->setCacheEnabled(true);

    if (!staticIp_.isEmpty())
    {
        QFont *fontStaticIp = FontManager::instance().getFont(13, false);
        captionStaticIpLayout_->setFont(*fontStaticIp);
        captionStaticIpLayout_->beginLayout();
        captionStaticIpLayout_->createLine();
        captionStaticIpLayout_->endLayout();
        captionStaticIpLayout_->setCacheEnabled(true);
    }

    timeMs_.recreateTextLayout();
}

} // namespace GuiLocations
