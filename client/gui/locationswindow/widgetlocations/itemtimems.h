#ifndef ITEMTIMEMS_H
#define ITEMTIMEMS_H

#include <QTextLayout>
#include "types/pingtime.h"

namespace GuiLocations {

// contains timeMs value and QTextLayout for it
class ItemTimeMs
{
public:
    ItemTimeMs(PingTime timeMs, bool bNeedCreateTextLayout);
    ~ItemTimeMs();

    void setValue(PingTime timeMs);
    PingTime getValue() const;
    QTextLayout *getTextLayout();

    static QString timeMsToString(PingTime timeMs);

    void recreateTextLayout();

private:
    PingTime timeMs_;
    QSharedPointer<QTextLayout> textLayout_;
    PingTime valueInTextLayout_;

    void recreateTextLayoutIfNeed();
};
} // namespace GuiLocations

#endif // ITEMTIMEMS_H
