#ifndef IUPDATEWINDOW_H
#define IUPDATEWINDOW_H

#include <QGraphicsObject>

class IUpdateWindow
{
public:
    virtual ~IUpdateWindow() {}

    virtual QGraphicsObject *getGraphicsObject() = 0;

    virtual void setVersion(QString version,int buildNumber) = 0;
    virtual void setProgress(int progressPercent) = 0;
    virtual void changeToDownloadingScreen() = 0;
    virtual void changeToPromptScreen() = 0;

    virtual void startAnimation() = 0;
    virtual void stopAnimation() = 0;

    virtual void updateScaling() = 0;

signals:
    virtual void acceptClick() = 0;
    virtual void cancelClick() = 0;
    virtual void laterClick() = 0;

};

Q_DECLARE_INTERFACE(IUpdateWindow, "IUpdateWindow")



#endif // IUPDATEWINDOW_H
