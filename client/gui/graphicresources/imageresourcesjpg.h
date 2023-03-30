#ifndef IMAGERESOURCESJPG_H
#define IMAGERESOURCESJPG_H

#include <QPixmap>
#include <QMutex>
#include <QHash>
#include "independentpixmap.h"

class ImageResourcesJpg
{
public:
    static ImageResourcesJpg &instance()
    {
        static ImageResourcesJpg ir;
        return ir;
    }

    void clearHash();

    QSharedPointer<IndependentPixmap> getIndependentPixmap(const QString &name, int width, int height);

private:
    ImageResourcesJpg();
    virtual ~ImageResourcesJpg();

    QHash<QString, QSharedPointer<IndependentPixmap> > hashIndependent_;

    struct FilenameDescr
    {
        QString filename;
        int width;
        int height;
    };

    QHash<QString, QVector<FilenameDescr> > filenames_;

    bool loadFromResourceWithCustomSize(const QString &name, int width, int height);
    bool parseFileName(const QString &filename, QString &outName, int &outWidth, int &outHeight);
    static bool sortSizesFunction(const FilenameDescr &f1, const FilenameDescr &f2);
};

#endif // IMAGERESOURCESJPG_H
