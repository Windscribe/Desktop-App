#pragma once

#include <QString>

namespace Help {

// Whether the given name is a command that has its own help page.
bool isCommand(const QString &command);

// The overview listing all commands.
QString overview();

// The help page of a single command. Falls back to the overview for an unknown command.
QString command(const QString &command);

// The usage lines of a single command, printed under an error about its arguments.
QString usage(const QString &command);

} // namespace Help
