#ifndef IMAGERESOURCESSVG_H
#define IMAGERESOURCESSVG_H

#include <QThread>
#include <QHash>
#include <QPixmap>
#include <QRecursiveMutex>
#include "independentpixmap.h"

class ImageResourcesSvg : public QThread
{
    Q_OBJECT

public:
    enum ImageFlags
    {
        IMAGE_FLAG_GRAYED = 1,
        IMAGE_FLAG_SQUARE = 2,
    };

    static ImageResourcesSvg &instance()
    {
        static ImageResourcesSvg ir;
        return ir;
    }

    void clearHashAndStartPreloading();
    void finishGracefully();

    QSharedPointer<IndependentPixmap> getIndependentPixmap(const QString &name);
    QSharedPointer<IndependentPixmap> getIconIndependentPixmap(const QString &name);

    QSharedPointer<IndependentPixmap> getFlag(const QString &flagName);
    QSharedPointer<IndependentPixmap> getScaledFlag(const QString &flagName, int width, int height, int flags = 0);

protected:
    void run() override;

private:
    ImageResourcesSvg();
    virtual ~ImageResourcesSvg();

    QHash<QString, QSharedPointer<IndependentPixmap> > iconHashes_;
    QHash<QString, QSharedPointer<IndependentPixmap> > hashIndependent_;
    std::atomic<bool> bNeedFinish_;
    bool bFininishedGracefully_;
    QRecursiveMutex mutex_;


    bool loadIconFromResource(const QString &name);
    bool loadFromResource(const QString &name);
    bool loadFromResourceWithCustomSize(const QString &name, int width, int height, int flags);
    QSharedPointer<IndependentPixmap> getIndependentPixmapScaled(const QString &name, int width, int height, int flags);
    void clearHash();
};

#endif // IMAGERESOURCESSVG_H
