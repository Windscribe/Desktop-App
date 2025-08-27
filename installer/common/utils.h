#pragma once

#include <QPixmap>

QPixmap getBorderedPixmap(const QString &path, int width, int height, int radius, bool roundTop = true, bool roundBottom = true);
