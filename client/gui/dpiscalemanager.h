#pragma once

#include <QObject>
#include <QScreen>
#include <QSet>

#define G_SCALE  (DpiScaleManager::instance().curScale())

class DpiScaleManager : public QObject
{
    Q_OBJECT

public:
    static DpiScaleManager &instance()
    {
        static DpiScaleManager s;
        return s;
    }

    inline double curScale() const
    {
        return curScale_;
    }

    int curDevicePixelRatio() const;
    QRect curScreenGeometry() const;
    double scaleOfScreen(const QScreen *screen) const;
    bool setMainWindow(QWidget *mainWindow);    // return true if DPI is changed

signals:
    void scaleChanged(double newScale);
    void screenConfigurationChanged(QSet<QScreen *> screens);
    void newScreen(QScreen *screen);

private slots:
    void onWindowScreenChanged(QScreen *screen);
    void onLogicalDotsPerInchChanged(qreal dpi);
    void onScreenAdded(QScreen *screen);

private:
    explicit DpiScaleManager();

private:
    static constexpr int kLowestLDPI = 96;
    double curScale_;
    qreal curDevicePixelRatio_;
    qreal curDPI_;
    QRect curGeometry_;
    QWidget *mainWindow_ = nullptr;
    QMetaObject::Connection screenChangedConnection_;
    QScreen* curScreen_;

    void setScale();
    void update(QScreen *screen);
};
