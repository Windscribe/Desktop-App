#include "itemtimems.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"


namespace GuiLocations {

ItemTimeMs::ItemTimeMs(PingTime timeMs, bool bNeedCreateTextLayout)
{
    timeMs_ = timeMs;

    if (bNeedCreateTextLayout)
    {
        recreateTextLayoutIfNeed();
    }
}

ItemTimeMs::~ItemTimeMs()
{
}

void ItemTimeMs::setValue(PingTime timeMs)
{
    timeMs_ = timeMs;
}

PingTime ItemTimeMs::getValue() const
{
    return timeMs_;
}

QTextLayout *ItemTimeMs::getTextLayout()
{
    recreateTextLayoutIfNeed();
    Q_ASSERT(!textLayout_.isNull());
    return textLayout_.data();
}

void ItemTimeMs::recreateTextLayout()
{
    QFont *font = FontManager::instance().getFont(11, false);
    textLayout_ = QSharedPointer<QTextLayout>(new QTextLayout(timeMsToString(timeMs_), *font));
    valueInTextLayout_ = timeMs_;

    textLayout_->beginLayout();
    textLayout_->createLine();
    textLayout_->endLayout();
    textLayout_->setCacheEnabled(true);
}

void ItemTimeMs::recreateTextLayoutIfNeed()
{
    if (!textLayout_.isNull())
    {
        if (valueInTextLayout_ != timeMs_)
        {
            textLayout_.reset();
        }
        else
        {
            // value not changed, so nothing todo and exit
            return;
        }
    }

    recreateTextLayout();
}

QString ItemTimeMs::timeMsToString(PingTime timeMs)
{
    if (timeMs.toInt() > 0)
    {
        return QString::number(timeMs.toInt());
    }
    else
    {
        return QObject::tr("N/A");
    }
}

} // namespace GuiLocations
