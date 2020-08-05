#include "cursorupdatehelper.h"

#ifdef Q_OS_MAC
#include "Utils/macutils.h"
#endif

namespace GuiLocations {

CursorUpdateHelper::CursorUpdateHelper(QWidget *widget)
    : widget_(widget), currentCursor_(Qt::PointingHandCursor)
{
    Q_ASSERT(widget_ != nullptr);
    widget_->setCursor(currentCursor_);
}

void CursorUpdateHelper::setForbiddenCursor()
{
    if (currentCursor_ != Qt::ForbiddenCursor) {
        currentCursor_ = Qt::ForbiddenCursor;
        applyCurrentCursor();
    }
}
void CursorUpdateHelper::setPointingHandCursor()
{
    if (currentCursor_ != Qt::PointingHandCursor) {
        currentCursor_ = Qt::PointingHandCursor;
        applyCurrentCursor();
    }
}

void CursorUpdateHelper::applyCurrentCursor()
{
    widget_->setCursor(currentCursor_);
#ifdef Q_OS_MAC
    auto *id = reinterpret_cast<void*>(widget_->effectiveWinId());
    MacUtils::invalidateCursorRects(id);
#endif
}
} // namespace GuiLocations
