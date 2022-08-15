#ifndef CURSORUPDATEHELPER_H
#define CURSORUPDATEHELPER_H

#include <QWidget>

namespace gui_location {

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

} // namespace gui_location

#endif // CURSORUPDATEHELPER_H
