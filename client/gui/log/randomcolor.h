/******************************************************************************
 * Copyright (c) 2015 Artur Eganyan
 *
 * This code is based on the randomColor project, which is
 * Copyright (c) 2014 David Merfield
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#ifndef RANDOMCOLOR_H
#define RANDOMCOLOR_H

#include <vector>
#include <random>
#include <initializer_list>

/**
 * If defined, makes the RandomColor methods thread-safe. It requires
 * support of std::mutex and related classes.
 */
//#define RANDOMCOLOR_THREAD_SAFE

#ifdef RANDOMCOLOR_THREAD_SAFE
#include <mutex>
#endif

/**
 * The RandomColor class represents a random color generator. It uses the
 * HSB color space and predefined color map to select colors. Example:
 *
 * \code
 * RandomColor randomColor;
 *
 * // Random color with normal luminosity
 * int c1 = randomColor.generate();
 *
 * // Random light red color
 * int c2 = randomColor.generate(RandomColor::Red, RandomColor::Light);
 *
 * // Random dark color with hue within [150; 160]
 * int c3 = randomColor.generate({150, 160}, RandomColor::Dark);
 *
 * // Random yellow or orange color with normal luminosity
 * int c4 = randomColor.generate({RandomColor::Yellow, RandomColor::Orange});
 * \endcode
 *
 * \note
 * By default, the methods are not thread-safe. Thread-safety depends on
 * the RANDOMCOLOR_THREAD_SAFE macro.
 */
class RandomColor
{
public:
    enum Color
    {
        Red,
        RedOrange,
        Orange,
        OrangeYellow,
        Yellow,
        YellowGreen,
        Green,
        GreenCyan,
        Cyan,
        CyanBlue,
        Blue,
        BlueMagenta,
        Magenta,
        MagentaPink,
        Pink,
        PinkRed,
        RandomHue,
        BlackAndWhite,
        Brown
    };

    enum Luminosity
    {
        Dark,
        Normal,
        Light,
        Bright,
        RandomLuminosity
    };

    typedef std::initializer_list<Color> ColorList;

    // Additional colors
    static const ColorList AnyRed;
    static const ColorList AnyOrange;
    static const ColorList AnyYellow;
    static const ColorList AnyGreen;
    static const ColorList AnyBlue;
    static const ColorList AnyMagenta;
    static const ColorList AnyPink;

    struct Range
    {
        Range( int value = 0 );
        Range( int left, int right );
        int& operator []( int i );
        int operator []( int i ) const;
        int size() const;

        int values[2];
    };

    /**
     * Returns a random color with specified luminosity, in RGB format
     * (0xRRGGBB).
     */
    int generate( Color color = RandomHue, Luminosity luminosity = Normal );

    /**
     * The same as generate(color, luminosity), but for one of the colors.
     * The color is selected randomly - the wider a color's hue range, the
     * higher chance it will be selected. The colors can be passed as
     * {color1, color2, ..., colorN}.
     */
    int generate( ColorList colors, Luminosity luminosity = Normal );

    /**
     * Returns a random color within hueRange, with specified luminosity,
     * in RGB format (0xRRGGBB). The range can be a pair {left, right} or
     * a single value (then it becomes {value, value}).
     */
    int generate( const Range& hueRange, Luminosity luminosity = Normal );

    /**
     * Sets the seed of the random generator.
     */
    void setSeed( int seed );

private:
    struct SBRange
    {
        SBRange( int s = 0, int bMin = 0, int bMax = 100 );

        int s;
        int bMin;
        int bMax;
    };

    struct ColorInfo
    {
        Color color;
        Range hRange;
        std::vector<SBRange> sbRanges;
    };

    int generate( int h, const ColorInfo&, Luminosity );
    int pickSaturation( const ColorInfo&, Luminosity );
    int pickBrightness( int s, const ColorInfo&, Luminosity );

    Range getBrightnessRange( int s, const ColorInfo& ) const;
    const ColorInfo& getColorInfo( int h ) const;
    int randomWithin( const Range& );
    int HSBtoRGB( double h, double s, double b ) const;

    static const std::vector<ColorInfo> colorMap;
    std::default_random_engine randomEngine;
#ifdef RANDOMCOLOR_THREAD_SAFE
    std::mutex mutex;
#endif
};


// RandomColor::Range

inline RandomColor::Range::Range( int left, int right )
    : values{left, right}
{}

inline RandomColor::Range::Range( int value )
    : values{value, value}
{}

inline int& RandomColor::Range::operator []( int i )
{
    return values[i];
}

inline int RandomColor::Range::operator []( int i ) const
{
    return values[i];
}

inline int RandomColor::Range::size() const
{
    return values[1] - values[0];
}

#endif
