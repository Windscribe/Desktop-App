#ifndef VOLUMEHELPER_H
#define VOLUMEHELPER_H

#include <QString>

class VolumeHelper_mac
{
public:
    explicit VolumeHelper_mac();

    const QString mountDmg(const QString &dmgFilename);
    bool unmountVolume(const QString &volumePath);
    const QString volumePath(const QString &dmgPathFilename);

};

#endif // VOLUMEHELPER_H
