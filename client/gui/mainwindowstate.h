#ifndef MAINWINDOWSTATE_H
#define MAINWINDOWSTATE_H

#include <QObject>

// singleton, global main window state(minimized/active)
class MainWindowState : public QObject
{
    Q_OBJECT

public:
    static MainWindowState &instance()
    {
        static MainWindowState s;
        return s;
    }

    bool isActive() const;
    void setActive(bool isActive);

signals:
    bool isActiveChanged(bool isActive);

private:
    MainWindowState();

private:
    bool isActive_;
};

#endif // MAINWINDOWSTATE_H
