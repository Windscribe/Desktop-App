#include <iostream>
#include "qconsolelistener.h"

QConsoleListener::QConsoleListener()
{
#ifdef Q_OS_WIN
    m_notifier = new QWinEventNotifier(GetStdHandle(STD_INPUT_HANDLE));
	QObject::connect(m_notifier, &QWinEventNotifier::activated, this, &QConsoleListener::readCommand);
#else
    m_notifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, this);
	QObject::connect(m_notifier, &QSocketNotifier::activated, this, &QConsoleListener::readCommand);
#endif  
}

#ifdef Q_OS_WIN
void QConsoleListener::readCommand(Qt::HANDLE hEvent)
#else
void QConsoleListener::readCommand(int socket)
#endif
{
#ifdef Q_OS_WIN
	Q_UNUSED(hEvent);
#else
	Q_UNUSED(socket);
#endif
    std::string line;
    std::getline(std::cin, line);
	QString strLine = QString::fromStdString(line);
	Q_EMIT this->newLine(strLine);
}
