/******************************************************************************
 * Copyright (c) 2015 Artur Eganyan
 *
 * This code is based on the randomColor project, which is
 * Copyright (c) 2014 David Merfield
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#include "randomcolor.h"
#include <cassert>

/**
 * The map is formed with the help of the following sources:
 * - The randomColor project by David Merfield
 * - http://www.workwithcolor.com/color-names-01.htm
 * - https://en.wikipedia.org/wiki/Lists_of_colors
 *
 * \note Colors must go in the same order as in Color enum. All colors after
 * RandomHue are special - they are not considered when the generator is
 * looking for the concrete hue value in the map.
 *
 * \note generate(RandomHue) does not use RandomHue saturation/brightness,
 * it is looking for more suitable parameters for the generated hue.
 */
const std::vector<RandomColor::ColorInfo> RandomColor::colorMap =
{
    //    color       hue range               saturation/brightness ranges as {s, bMin, bMax = 100}, s must increase
    { Red,            {-5, 10},    {{20, 100}, {30, 95}, {40, 90}, {50, 88}, {60, 80}, {70, 70}, {80, 60}, {90, 58}, {100, 55}} },
    { RedOrange,      {11, 19},    {{20, 100}, {30, 95}, {40, 90}, {50, 85}, {60, 80}, {70, 70}, {80, 60}, {90, 60}, {100, 55}} },
    { Orange,         {20, 41},    {{25, 100}, {30, 93}, {40, 88}, {50, 86}, {60, 85}, {70, 70}, {100, 70}} },
    { OrangeYellow,   {42, 50},    {{25, 100}, {30, 93}, {40, 88}, {50, 86}, {60, 85}, {70, 70}, {100, 70}} },
    { Yellow,         {51, 61},    {{28, 100}, {40, 98}, {50, 95}, {60, 93}, {70, 85}, {80, 82}, {90, 78}, {100, 75}} },
    { YellowGreen,    {62, 75},    {{30, 100}, {40, 94}, {50, 90}, {60, 86}, {70, 84}, {80, 82}, {90, 78}, {100, 75}} },
    { Green,          {76, 140},   {{30, 100}, {40, 95}, {50, 85}, {60, 75}, {70, 60}, {80, 54}, {90, 48}, {100, 45}} },
    { GreenCyan,      {141, 171},  {{20, 100}, {30, 95}, {40, 90}, {50, 85}, {60, 80}, {70, 60}, {80, 55}, {90, 52}, {100, 45}} },
    { Cyan,           {172, 200},  {{20, 100}, {30, 90}, {40, 80}, {50, 74}, {60, 58}, {70, 52}, {90, 52}, {100, 50}} },
    { CyanBlue,       {201, 215},  {{20, 100}, {30, 90}, {40, 80}, {50, 74}, {60, 58}, {70, 52}, {90, 52}, {100, 50}} },
    { Blue,           {216, 240},  {{20, 100}, {30, 90}, {40, 85}, {50, 76}, {60, 65}, {70, 64}, {80, 60}, {90, 55}, {100, 55}} },
    { BlueMagenta,    {241, 280},  {{20, 100}, {30, 90}, {40, 85}, {50, 76}, {60, 65}, {70, 64}, {80, 60}, {90, 55}, {100, 55}} },
    { Magenta,        {281, 315},  {{20, 100}, {30, 85}, {40, 80}, {50, 70}, {60, 60}, {70, 55}, {100, 55}} },
    { MagentaPink,    {316, 330},  {{20, 100}, {30, 95}, {40, 92}, {50, 87}, {60, 84}, {80, 70}, {90, 65}, {100, 65}} },
    { Pink,           {331, 340},  {{20, 100}, {30, 95}, {40, 92}, {50, 87}, {60, 84}, {80, 80}, {90, 75}, {100, 73}} },
    { PinkRed,        {341, 354},  {{20, 100}, {30, 95}, {40, 90}, {50, 85}, {60, 80}, {70, 70}, {80, 60}, {90, 58}, {100, 55}} },

    { RandomHue,      {0, 359},    {{20, 100}, {100, 50}} },
    { BlackAndWhite,  {0, 359},    {{0, 0, 100}} },
    { Brown,          {15, 30},    {{20, 90, 95}, {30, 80, 90}, {40, 60, 90}, {50, 50, 90}, {60, 50, 90}, {70, 50, 90}, {80, 45, 85}, {90, 45, 85}, {100, 40, 85}} }
};

using ColorList = RandomColor::ColorList;
const ColorList RandomColor::AnyRed     = { PinkRed, Red, RedOrange };
const ColorList RandomColor::AnyOrange  = { RedOrange, Orange, OrangeYellow };
const ColorList RandomColor::AnyYellow  = { OrangeYellow, Yellow, YellowGreen };
const ColorList RandomColor::AnyGreen   = { YellowGreen, Green, GreenCyan };
const ColorList RandomColor::AnyBlue    = { Cyan, CyanBlue, Blue, BlueMagenta };
const ColorList RandomColor::AnyMagenta = { BlueMagenta, Magenta, MagentaPink };
const ColorList RandomColor::AnyPink    = { MagentaPink, Pink, PinkRed };


inline RandomColor::SBRange::SBRange( int s_, int bMin_, int bMax_ )
    : s(s_), bMin(bMin_), bMax(bMax_)
{}


int RandomColor::generate( Color color, Luminosity luminosity )
{
    const ColorInfo& info = colorMap[color];
    const int h = randomWithin(info.hRange);
    if (color != RandomHue) {
        return generate(h, info, luminosity);
    } else {
        return generate(h, getColorInfo(h), luminosity);
    }
}

int RandomColor::generate( ColorList colors, Luminosity luminosity )
{
    // Each color will have probability = color.hRange.size() / totalRangeSize,
    // so calculate totalRangeSize
    double totalRangeSize = 0;
    for (Color color : colors) {
        totalRangeSize += colorMap[color].hRange.size();
    }

    // Generate a number within [0; 1) and select a color according with the
    // cumulative distribution function f(i) = f(i - 1) + colorProbability(i)
    std::uniform_real_distribution<double> probability(0, 1);
    double p = probability(randomEngine);
    double f = 0;
    for (Color color : colors) {
        const double colorProbability = colorMap[color].hRange.size() / totalRangeSize;
        f += colorProbability;
        if (f >= p) {
            return generate(color, luminosity);
        }
    }

    // This place can be reached due to rounding (if p ~ 1 and f < p even
    // for the last color). Then return the last color.
    Color lastColor = *(colors.end() - 1);
    return generate(lastColor, luminosity);
}

int RandomColor::generate( const Range& hueRange, Luminosity luminosity )
{
    const int h = randomWithin(hueRange);
    const ColorInfo& info = getColorInfo(h);
    return generate(h, info, luminosity);
}

int RandomColor::generate( int h, const ColorInfo& info, Luminosity luminosity )
{
    const int s = pickSaturation(info, luminosity);
    const int b = pickBrightness(s, info, luminosity);
    return HSBtoRGB(h, s, b);
}

int RandomColor::pickSaturation( const ColorInfo& info, Luminosity luminosity )
{
    Range sRange = {info.sbRanges[0].s, info.sbRanges.back().s};
    const int sRangeSize = sRange.size();

    switch (luminosity) {
        case Dark:
            sRange[0] = sRange[1] - sRangeSize * 0.3;
            break;

        case Light:
            sRange[1] = sRange[0] + sRangeSize * 0.25;
            break;

        case Bright:
            sRange[0] = sRange[1] - sRangeSize * 0.3;
            break;

        case Normal:
            sRange[0] = std::max(sRange[0], std::min(50, sRange[1]));
            break;

        case RandomLuminosity:
            // Just use the whole range
            break;
    }
    return randomWithin(sRange);
}

int RandomColor::pickBrightness( int s, const ColorInfo& info, Luminosity luminosity )
{
    Range bRange = getBrightnessRange(s, info);
    const int bRangeSize = bRange.size();

    switch (luminosity) {
        case Dark:
            bRange[1] = bRange[0] + std::min(bRangeSize * 0.3, 30.0);
            break;

        case Light:
            bRange[0] = bRange[1] - std::min(bRangeSize * 0.3, 15.0);
            break;

        case Bright:
            bRange[0] = bRange[1] - std::min(bRangeSize * 0.3, 10.0);
            break;

        case Normal:
            bRange[0] += bRangeSize * 0.5;
            bRange[1] -= bRangeSize * 0.125;
            break;

        case RandomLuminosity:
            // Just use the whole range
            break;
    }
    return randomWithin(bRange);
}

RandomColor::Range RandomColor::getBrightnessRange( int s, const ColorInfo& info ) const
{
    const std::vector<SBRange>& sbRanges = info.sbRanges;

    // Find a slice of sbRanges to which s belongs, and calculate the
    // brightness range proportionally to s
    for (int i = sbRanges.size() - 2; i >= 0; -- i) {
        if (s >= sbRanges[i].s) {
            const SBRange& r1 = sbRanges[i];
            const SBRange& r2 = sbRanges[i + 1];
            const double sRangeSize = r2.s - r1.s;
            const double sFraction = sRangeSize ? (s - r1.s) / sRangeSize : 0;
            const int bMin = r1.bMin + sFraction * (r2.bMin - r1.bMin);
            const int bMax = r1.bMax + sFraction * (r2.bMax - r1.bMax);
            return {bMin, bMax};
        }
    }
    if (sbRanges.size() == 1) {
       return {sbRanges[0].bMin, sbRanges[0].bMax};
    }
    return {0, 100};
}

const RandomColor::ColorInfo& RandomColor::getColorInfo( int h ) const
{
    if (h < 0) h += 360;

    // Find the narrowest range containing h
    const ColorInfo* found = &colorMap[RandomHue];
    for (const ColorInfo& info : colorMap) {
        if ((info.hRange[0] <= h && h <= info.hRange[1]) ||
            (info.hRange[0] <= 0 && h >= 360 + info.hRange[0])) {
            if (info.hRange.size() < found->hRange.size() && info.color <= RandomHue) {
                found = &info;
            }
        }
    }
    return *found;
}

int RandomColor::randomWithin( const Range& range )
{
#ifdef RANDOMCOLOR_THREAD_SAFE
    std::lock_guard<std::mutex> lock(mutex);
#endif
    std::uniform_int_distribution<int> d(range[0], range[1]);
    return d(randomEngine);
}

void RandomColor::setSeed( int seed )
{
#ifdef RANDOMCOLOR_THREAD_SAFE
    std::lock_guard<std::mutex> lock(mutex);
#endif
    randomEngine.seed(seed);
}

// Parameter "b" is renamed as "v" (value) for convenience
int RandomColor::HSBtoRGB( double h, double s, double v ) const
{
    assert(-360 <= h && h <= 360 && "h must be within [-360; 360]");
    assert(   0 <= s && s <= 100 && "s must be within [0; 100]");
    assert(   0 <= v && v <= 100 && "v must be within [0; 100]");

    // Keep h within [0; 359], s and v within [0; 1]
    if (h >= 360) h -= 360;
    if (h < 0)    h += 360;
    s = s / 100.0;
    v = v / 100.0;

    // Convert hsv to rgb. This algorithm is described at
    // https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV
    const double C = v * s;
    const double hd = h / 60;
    const int hi = int(hd);
    const double hd_mod2 = hd - hi + hi % 2;
    const double X = C * (1 - fabs(hd_mod2 - 1));
    double r, g, b;

    switch (hi) {
        case 0:  r = C; g = X; b = 0;  break;
        case 1:  r = X; g = C; b = 0;  break;
        case 2:  r = 0; g = C; b = X;  break;
        case 3:  r = 0; g = X; b = C;  break;
        case 4:  r = X; g = 0; b = C;  break;
        case 5:  r = C; g = 0; b = X;  break;
        // This should never happen
        default: return 0;
    }

    const double m = v - C;
    r += m;
    g += m;
    b += m;

    // Scale r, g, b to [0; 255] and pack them into a number 0xRRGGBB
    return (int(r * 255) << 16) + (int(g * 255) << 8) + int(b * 255);
}
