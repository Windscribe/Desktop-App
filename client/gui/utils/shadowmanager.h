#pragma once

#include <QObject>
#include <QPixmap>

// todo: maybe optimize for better speed
class ShadowManager : public QObject
{
    Q_OBJECT
public:
    enum SHAPE_ID { SHAPE_ID_INVALID = -1, SHAPE_ID_LOCATIONS, SHAPE_ID_CONNECT_WINDOW, SHAPE_ID_LOGIN_WINDOW, SHAPE_ID_PREFERENCES,
                    SHAPE_ID_BOTTOM_INFO, SHAPE_ID_UPDATE_WIDGET, SHAPE_ID_INIT_WINDOW, SHAPE_ID_NEWS_FEED, SHAPE_ID_PROTOCOL,
                    SHAPE_ID_GENERAL_MESSAGE, SHAPE_ID_EXIT };

    explicit ShadowManager(QObject *parent = nullptr);

    void addPixmap(const QPixmap &pixmap, int x, int y, int id, bool isVisible);
    void addRectangle(const QRect &rc, int id, bool isVisible);
    void changeRectangleSize(int id, const QRect &newRc);
    void changePixmapPos(int id, int x, int y);
    void setVisible(int id, bool isVisible);
    void setOpacity(int id, qreal opacity, bool withUpdateShadow);
    void removeObject(int id);

    bool isInShadowList(int id) const;

    QPixmap &getCurrentShadowPixmap();
    int getShadowMargin() const;

    bool isNeedShadow() const;
    void updateShadow();

signals:
    void shadowUpdated();

private:
    static constexpr int SHADOW_MARGIN = 30;
    QPixmap currentShadow_;

    struct ShadowObject
    {
        int id;
        bool isVisible;
        qreal opacity;
        int posX;
        int posY;
        QPixmap pixmap;
    };


    QVector<ShadowObject> objects_;

    QRect calcBoundingRect();
    int findObjectIndById(int id) const;
};
