#ifndef HOSTNAMESRESOLVER_H
#define HOSTNAMESRESOLVER_H

#include <QObject>

namespace locationsmodel {

class HostnamesResolver : public QObject
{
    Q_OBJECT
public:
    explicit HostnamesResolver(QObject *parent = nullptr);

    void setHostnames(const QStringList &hostnames);

signals:

};

} //namespace locationsmodel

#endif // HOSTNAMESRESOLVER_H
