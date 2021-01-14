#ifndef COMMONGRAPHICS_H
#define COMMONGRAPHICS_H

#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <QColor>
#include <QFont>
#include <QPixmap>

const int WINDOW_WIDTH = 332;
const int WINDOW_HEIGHT = 316;
const int WINDOW_MARGIN = 16;

const int LOCATIONS_TAB_HEIGHT = 397;

const int LOGIN_WIDTH = 332;
const int LOGIN_HEIGHT = 354;

const int LOGIN_BUTTON_POS_Y                 = 232;

const double OPACITY_HIDDEN                  = 0.0;
const double OPACITY_UNHOVER_DIVIDER         = 0.1;
const double OPACITY_UNHOVER_DIVIDER_DARK    = 0.2;
const double OPACITY_UNHOVER_ICON_TEXT       = 0.2;
const double OPACITY_UNHOVER_ICON_TEXT_DARK  = 0.3;
const double OPACITY_UNHOVER_ICON_STANDALONE = 0.4;
const double OPACITY_UNHOVER_TEXT            = 0.4;
const double OPACITY_HALF                    = 0.5;
const double OPACITY_FULL                    = 1.0;

const double OPACITY_BACKGROUND_PROGRESS     = 0.25;
const double OPACITY_NEWSFEED_HEADER_BACKGROUND = 0.15;
const double OPACTIY_NEWSFEED_BACKGROUND        = 0.96;
const double OPACITY_BACKGROUND_OVERLAY_WHITE   = 1.00;
const double OPACITY_EMERGENCY_BACKGROUND       = 0.96;
const double OPACITY_OVERLAY_BACKGROUND    = 0.96;

const int ANIMATION_SPEED_VERY_FAST = 50;
const int ANIMATION_SPEED_FAST      = 100;
const int ANIMATION_SPEED_SLOW      = 300;
const int ANIMATION_SPEED_VERY_SLOW = 900;

template<typename INT_OR_DOUBLE>
void startAnAnimation(QVariantAnimation &animation, INT_OR_DOUBLE start, INT_OR_DOUBLE stop, int duration)
{
    animation.stop();
    animation.setStartValue(start);
    animation.setEndValue(stop);
    animation.setDuration(duration);
    animation.start();
}

template<typename QCOLOR>
void startColorAnimation(QPropertyAnimation &animation, QCOLOR startColor, QCOLOR stopColor, int duration)
{
    animation.stop();
    animation.setStartValue(startColor);
    animation.setEndValue(stopColor);
    animation.setDuration(duration);
    animation.start();
}

namespace CommonGraphics{

int centeredOffset(int background_length, int graphic_length);

int textWidth(QString text, const QFont &font);
int textHeight(const QFont &font);

int idealWidth(int minWidth, int maxWidth, int idealLines, int textOneLineWidth);
int idealWidthOfTextRect(int minWidth, int maxWidth, int idealLines, QString text, const QFont &font);
QRect idealRect(int posX, int posY, int minWidth, int maxWidth, int idealLines, QString text, QFont font, Qt::TextFlag flag);

QRect textBoundingRect(QFont font, int posX, int posY, int width, QString message, Qt::TextFlag flag);

QString truncateText(QString original, QFont font, int width);
}




#endif // COMMONGRAPHICS_H
