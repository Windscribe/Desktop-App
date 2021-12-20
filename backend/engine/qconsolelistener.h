#pragma once

#include <QObject>
#include <iostream>

#ifdef Q_OS_WIN
#include <QWinEventNotifier>
#include <windows.h>
#else
#include <QSocketNotifier>
#endif

class QConsoleListener : public QObject
{
    Q_OBJECT

public:
    QConsoleListener();

Q_SIGNALS:
    void newLine(const QString &strNewLine);

private:
#ifdef Q_OS_WIN
    QWinEventNotifier *m_notifier;
#else
    QSocketNotifier *m_notifier;
#endif

private Q_SLOTS:
#ifdef Q_OS_WIN
	void readCommand(Qt::HANDLE hEvent);
#else
	void readCommand(int socket);
#endif
};