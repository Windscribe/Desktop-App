#ifndef IMAGERESOURCESSVG_H
#define IMAGERESOURCESSVG_H

#include <QPixmap>
#include "independentpixmap.h"

class ImageResourcesSvg
{

public:
    static ImageResourcesSvg &instance()
    {
        static ImageResourcesSvg ir;
        return ir;
    }

    void updateHashedPixmaps();

    // new interface
    void clearHash();
    IndependentPixmap *getIndependentPixmap(const QString &name);
    IndependentPixmap *getIconIndependentPixmap(const QString &name);

    IndependentPixmap *getFlag(const QString &flagName);
    IndependentPixmap *getScaledFlag(const QString &flagName, int width, int height);

private:
    ImageResourcesSvg();
    virtual ~ImageResourcesSvg();

    void clearHashes();

    QHash<QString, QPixmap *> hash_;
    QHash<int, QHash<QString, QPixmap *> > scaledHashes_;

    QHash<QString, IndependentPixmap *> iconHashes_;
    bool loadIconFromResource(const QString &name);

    QHash<QString, IndependentPixmap *> hashIndependent_;
    bool loadFromResource(const QString &name);
    bool loadFromResourceWithCustomSize(const QString &name, int width, int height);

    IndependentPixmap *getIndependentPixmapScaled(const QString &name, int width, int height);

};

#endif // IMAGERESOURCESSVG_H
