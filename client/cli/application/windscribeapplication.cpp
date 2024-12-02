#include "windscribeapplication.h"

#include <QAbstractEventDispatcher>
#include <unistd.h>

#include "utils/log/categories.h"

WindscribeApplication::WindscribeApplication(int &argc, char **argv) : QCoreApplication(argc, argv)
{
}

WindscribeApplication::~WindscribeApplication()
{
}

void WindscribeApplication::setService(MainService *service)
{
    service_ = service;
}

void WindscribeApplication::setSigTermHandler(int fd)
{
    fd_ = fd;

    socketNotifier_ = new QSocketNotifier(fd_, QSocketNotifier::Read, this);
    connect(socketNotifier_, &QSocketNotifier::activated, this, &WindscribeApplication::onSigTerm);
}

void WindscribeApplication::onSigTerm()
{
    socketNotifier_->setEnabled(false);
    char tmp;
    if (::read(fd_, &tmp, sizeof(tmp)) < 0) {
        qCCritical(LOG_BASIC) << "Could not read from signal socket";
        return;
    }

    service_->stop();
    exit(0);
}

