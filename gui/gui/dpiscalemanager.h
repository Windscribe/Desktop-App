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

    bool setMainWindow(QWidget *mainWindow);    // return true if DPI is changed



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

    double scaleOfScreen(const QScreen *screen) const;

signals:
    void scaleChanged(double newScale);
    void screenConfigurationChanged(QSet<QScreen *> screens);
    void newScreen(QScreen *screen);

private slots:
    void onWindowScreenChanged(QScreen *screen);
    void onLogicalDotsPerInchChanged(qreal dpi);

private:
    explicit DpiScaleManager();

private:
    static constexpr int LOWEST_LDPI = 96;
    double curScale_;
    int curDevicePixelRatio_;
    int curDPI_;
    QRect curGeometry_;
    QWidget *mainWindow_;

};

#endif // DPISCALEMANAGER_H
