#pragma once

#include <QString>

namespace Utils {

// Whether stdin is a terminal, so a prompt can be answered. Agents and scripts run without one.
bool isInteractive();
// Reads one line from stdin without a prompt, for input piped in. Empty at the end of the input.
QString readLine();
// Whether getInput() can hide what is typed. Not in mintty without ConPTY, whose echo only Cygwin itself controls.
bool canDisableEcho();
QString getInput(const QString &prompt, bool disableEcho);

} // namespace Utils