#ifndef NUMBERSPIXMAP_H
#define NUMBERSPIXMAP_H

#include <QFont>
#include "GraphicResources/independentpixmap.h"


class NumbersPixmap
{
public:
    NumbersPixmap();
    virtual ~NumbersPixmap();

    int width() const;
    int height() const;

    QFont *getFont();
    IndependentPixmap *getPixmap();
    IndependentPixmap *getDotPixmap();
    IndependentPixmap *getNAPixmap();

    int dotWidth() const;

    void rescale();

private:
    IndependentPixmap *pixmap_;
    IndependentPixmap *dotPixmap_;
    IndependentPixmap *naPixmap_;
    QFont font_;
    int itemWidth_;
    int itemHeight_;
    int dotWidth_;
};

#endif // NUMBERSPIXMAP_H
