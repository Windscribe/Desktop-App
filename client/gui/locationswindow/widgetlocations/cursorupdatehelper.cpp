#include "cursorupdatehelper.h"

#ifdef Q_OS_MAC
#include "utils/macutils.h"
#endif

namespace GuiLocations {

CursorUpdateHelper::CursorUpdateHelper(QWidget *widget)
    : widget_(widget), currentCursor_(Qt::PointingHandCursor)
{
    Q_ASSERT(widget_ != nullptr);
    widget_->setCursor(currentCursor_);
}

void CursorUpdateHelper::setArrowCursor()
{
    if (currentCursor_ != Qt::ArrowCursor) {
        currentCursor_ = Qt::ArrowCursor;
        applyCurrentCursor();
    }
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
