#pragma once

#include <QWidget>

namespace gui_locations {

class CursorUpdateHelper
{
public:
    explicit CursorUpdateHelper(QWidget *widget);

    void setArrowCursor();
    void setForbiddenCursor();
    void setPointingHandCursor();

private:
    void applyCurrentCursor();

    QWidget *widget_;
    Qt::CursorShape currentCursor_;
};

} // namespace gui_locations

