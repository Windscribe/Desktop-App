#pragma once

#include <QCoreApplication>
#include <QSocketNotifier>

#include "../mainservice.h"

class WindscribeApplication : public QCoreApplication
{
    Q_OBJECT
public:
    explicit WindscribeApplication(int &argc, char **argv);
    ~WindscribeApplication();

    static WindscribeApplication *instance() {
        return static_cast<WindscribeApplication *>(qApp->instance());
    }

    void setService(MainService *service);
    void setSigTermHandler(int fd);

private slots:
    void onSigTerm();

private:
    MainService *service_;

    QSocketNotifier *socketNotifier_;
    int fd_;
};
