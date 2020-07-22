#ifndef DPISCALEMANAGER_H
#define DPISCALEMANAGER_H

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

    void init(QWidget *mainWindow);
    inline double curScale() const
    {
        return curScale_;
    }

    inline int curDevicePixelRatio() const
    {
        return curDevicePixelRatio_;
    }

    inline QRect curScreenGeometry() const
    {
        return curGeometry_;
    }

signals:
    void scaleChanged(double newScale);
    void screenConfigurationChanged(QSet<QScreen *> screens);

private slots:
    void onWindowScreenChanged(QScreen *screen);
    void onLogicalDotsPerInchChanged(qreal dpi);
    void onScreenAdded(QScreen *screen);
    void onScreenRemoved(QScreen *screen);

private:
    explicit DpiScaleManager();

private:
    const int LOWEST_LDPI = 96;
    double curScale_;
    int curDevicePixelRatio_;
    int curDPI_;
    QRect curGeometry_;
    QSet<QScreen *> screens_;
    QScreen *appCurScreen_;
    QWidget *mainWindow_;

};

#endif // DPISCALEMANAGER_H
