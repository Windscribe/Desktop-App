#include "shadowmanager.h"

#include <QPainter>
#include "utils/ws_assert.h"
#include "utils/makecustomshadow.h"

extern qreal g_pixelRatio;

ShadowManager::ShadowManager(QObject *parent) : QObject(parent)
{

}

void ShadowManager::addPixmap(const QPixmap &pixmap, int x, int y, int id, bool isVisible)
{
    if (!isNeedShadow())
    {
        return;
    }

    ShadowObject so;
    so.id = id;
    so.isVisible = isVisible;
    so.opacity = 1.0;
    so.pixmap = pixmap;
    so.posX = x;
    so.posY = y;
    objects_ << so;
    if (isVisible)
    {
        updateShadow();
    }
}

void ShadowManager::addRectangle(const QRect &rc, int id, bool isVisible)
{
    if (!isNeedShadow())
    {
        return;
    }

    ShadowObject so;
    so.id = id;
    so.isVisible = isVisible;
    so.opacity = 1.0;
    so.pixmap = QPixmap(rc.width(), rc.height());
    so.pixmap.fill(QColor(0, 0, 0));
    so.posX = rc.left();
    so.posY = rc.top();
    objects_ << so;
    if (isVisible)
    {
        updateShadow();
    }
}

void ShadowManager::changeRectangleSize(int id, const QRect &newRc)
{
    if (!isNeedShadow())
    {
        return;
    }

    int ind = findObjectIndById(id);
    if (ind == -1)
    {
        WS_ASSERT(false);
        return;
    }

    if (objects_[ind].pixmap.width() != newRc.width() || objects_[ind].pixmap.height() != newRc.height())
    {
        objects_[ind].pixmap = QPixmap(newRc.width(), newRc.height());
        objects_[ind].pixmap.fill(QColor(0, 0, 0));
    }
    objects_[ind].posX = newRc.left();
    objects_[ind].posY = newRc.top();

    if (objects_[ind].isVisible)
    {
        updateShadow();
    }
}

void ShadowManager::changePixmapPos(int id, int x, int y)
{
    if (!isNeedShadow())
    {
        return;
    }

    int ind = findObjectIndById(id);
    if (ind == -1)
    {
        return;
    }

    if (objects_[ind].posX != x || objects_[ind].posX != y)
    {
        objects_[ind].posX = x;
        objects_[ind].posY = y;
        if (objects_[ind].isVisible)
        {
            updateShadow();
        }
    }
}

void ShadowManager::setVisible(int id, bool isVisible)
{
    if (!isNeedShadow())
    {
        return;
    }

    int ind = findObjectIndById(id);
    if (ind == -1)
    {
        WS_ASSERT(false);
        return;
    }

    if (objects_[ind].isVisible != isVisible)
    {
        objects_[ind].isVisible = isVisible;
        updateShadow();
    }
}

void ShadowManager::setOpacity(int id, qreal opacity, bool withUpdateShadow)
{
    if (!isNeedShadow())
    {
        return;
    }

    int ind = findObjectIndById(id);
    if (ind == -1)
    {
        WS_ASSERT(false);
        return;
    }

    objects_[ind].opacity = opacity;
    if (withUpdateShadow && objects_[ind].isVisible)
    {
        updateShadow();
    }
}

void ShadowManager::removeObject(int id)
{
    if (!isNeedShadow())
    {
        return;
    }

    int ind = findObjectIndById(id);
    if (ind != -1)
    {
        objects_.remove(ind);
        updateShadow();
    }
}

bool ShadowManager::isInShadowList(int id) const
{
    return findObjectIndById(id) != -1;
}

QPixmap &ShadowManager::getCurrentShadowPixmap()
{
    WS_ASSERT(isNeedShadow());
    return currentShadow_;
}

int ShadowManager::getShadowMargin() const
{
 #if defined(Q_OS_WIN)
    return SHADOW_MARGIN;
#else
    return 0;
#endif
}

bool ShadowManager::isNeedShadow() const
{
#if defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
}

QRect ShadowManager::calcBoundingRect()
{
    if (objects_.size() > 0)
    {
        QRect rc(objects_[0].posX, objects_[0].posY, objects_[0].pixmap.width(), objects_[0].pixmap.height());

        for (int i = 1; i < objects_.size(); ++i)
        {
            rc = rc.united(QRect(objects_[i].posX, objects_[i].posY, objects_[i].pixmap.width(), objects_[i].pixmap.height()));
        }
        return rc;
    }
    else
    {
        return QRect();
    }
}

void ShadowManager::updateShadow()
{
    if (!isNeedShadow())
    {
        return;
    }

    QRect rcBounding = calcBoundingRect();

    QPixmap pixmap(rcBounding.width() , rcBounding.height() );
    pixmap.fill(Qt::transparent);

    {
        QPainter painter(&pixmap);
        for (const ShadowObject &so : std::as_const(objects_))
        {
            if (so.isVisible)
            {
                painter.setOpacity(so.opacity);
                painter.drawPixmap(so.posX, so.posY, so.pixmap);
            }
        }
    }

    currentShadow_ = MakeCustomShadow::makeShadowForPixmap(pixmap, SHADOW_MARGIN, SHADOW_MARGIN, QColor(0, 0, 0, 225));

    emit shadowUpdated();

}

int ShadowManager::findObjectIndById(int id) const
{
    for (int i = 0; i < objects_.count(); ++i)
    {
        if (objects_[i].id == id)
        {
            return i;
        }
    }
    return -1;
}
