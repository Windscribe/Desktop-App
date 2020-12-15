#ifndef IMAGERESOURCESSVG_H
#define IMAGERESOURCESSVG_H

#include <QThread>
#include <QPixmap>
#include <QMutex>
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

    IndependentPixmap *getIndependentPixmap(const QString &name);
    IndependentPixmap *getIconIndependentPixmap(const QString &name);

    IndependentPixmap *getFlag(const QString &flagName);
    IndependentPixmap *getScaledFlag(const QString &flagName, int width, int height, int flags = 0);

protected:
    void run() override;

private:
    ImageResourcesSvg();
    virtual ~ImageResourcesSvg();

    QHash<QString, IndependentPixmap *> iconHashes_;
    QHash<QString, IndependentPixmap *> hashIndependent_;
    std::atomic<bool> bNeedFinish_;
    bool bFininishedGracefully_;
    QMutex mutex_;


    bool loadIconFromResource(const QString &name);
    bool loadFromResource(const QString &name);
    bool loadFromResourceWithCustomSize(const QString &name, int width, int height, int flags);
    IndependentPixmap *getIndependentPixmapScaled(const QString &name, int width, int height, int flags);
    void clearHash();
};

#endif // IMAGERESOURCESSVG_H
