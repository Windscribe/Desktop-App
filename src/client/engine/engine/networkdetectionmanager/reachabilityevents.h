#pragma once

#include <QObject>

class ReachAbilityEvents : public QObject
{
    Q_OBJECT
public:

    static ReachAbilityEvents &instance()
    {
        static ReachAbilityEvents s;
        return s;
    }

    void init();

    explicit ReachAbilityEvents(QObject *parent = 0);
    virtual ~ReachAbilityEvents();

    void emitNetworkChanged();

signals:
    void networkStateChanged();

};
