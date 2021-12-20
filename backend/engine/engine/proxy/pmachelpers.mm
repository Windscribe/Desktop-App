#include "pmachelpers.h"
#include <QDebug>
#import "pmachelpers.h"

QVariantMap pMacHelpers::toQVariantMap( CFDictionaryRef dict )
{
    QVariantMap map;

    if ( dict ) {
        const CFIndex count = CFDictionaryGetCount( dict );
        const void* keys[ count ];
        const void* values[ count ];

        CFDictionaryGetKeysAndValues( dict, keys, values );

        for ( CFIndex i = 0; i < count; i++ ) {
            const QVariant key = toQVariant( (CFTypeRef)keys[ i ] );
            const QVariant value = toQVariant( (CFTypeRef)values[ i ] );

            map[ key.toString() ] = value;
        }
    }

    return map;
}

QVariantList pMacHelpers::toQVariantList( CFArrayRef array )
{
    QVariantList list;

    if ( array ) {
        const CFIndex count = CFArrayGetCount( array );

        for ( CFIndex i = 0; i < count; i++ ) {
            list << toQVariant( CFArrayGetValueAtIndex( array, i ) );
        }
    }

    return list;
}

QVariant pMacHelpers::toQVariant( CFStringRef string )
{
    if ( string ) {
        const CFIndex length = 2 *( CFStringGetLength( string ) +1 ); // Worst case for UTF8
        char buffer[ length ];

        if ( CFStringGetCString( string, buffer, length, kCFStringEncodingUTF8 ) ) {
            return QString::fromUtf8( buffer );
        }
        else {
            qWarning() << Q_FUNC_INFO << "CFStringRef conversion failed";
        }
    }

    return QVariant();
}

QVariant pMacHelpers::toQVariant( CFBooleanRef value )
{
    return value ? (bool)CFBooleanGetValue( value ) : QVariant();
}

QVariant pMacHelpers::toQVariant( CFNumberRef number )
{
    switch ( CFNumberGetType( number ) ) {
        case kCFNumberSInt8Type:
        case kCFNumberSInt16Type:
        case kCFNumberSInt32Type:
        case kCFNumberSInt64Type:
        case kCFNumberCharType:
        case kCFNumberShortType:
        case kCFNumberIntType:
        case kCFNumberLongType:
        case kCFNumberLongLongType:
        case kCFNumberCFIndexType:
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
        case kCFNumberNSIntegerType:
#endif
        {
            qint64 value = 0;
            if ( CFNumberGetValue( number, kCFNumberSInt64Type, &value ) ) {
                return value;
            }
            break;
        }
        case kCFNumberFloat32Type:
        case kCFNumberFloat64Type:
        case kCFNumberFloatType:
        case kCFNumberDoubleType:
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
        case kCFNumberCGFloatType:
#endif
        {
            qreal value = 0;
            if ( CFNumberGetValue( number, kCFNumberFloat64Type, &value ) ) {
                return value;
            }
            break;
        }
    }

    return QVariant();
}

QVariant pMacHelpers::toQVariant( CFDataRef data )
{
    return data ? QByteArray( (const char*)CFDataGetBytePtr( data ), CFDataGetLength( data ) ) : QVariant();
}

QVariant pMacHelpers::toQVariant( CFURLRef url )
{
    if ( url ) {
        const CFStringRef string = CFURLCopyFileSystemPath( url, kCFURLPOSIXPathStyle );
        const QVariant variant = toQVariant( string );
        CFRelease( string );
        return variant;
    }

    return QVariant();
}

QVariant pMacHelpers::toQVariant( CFUUIDRef uuid )
{
    if ( uuid ) {
        const CFStringRef string = CFUUIDCreateString( kCFAllocatorDefault, uuid );
        const QVariant variant = toQVariant( string );
        CFRelease( string );
        return variant;
    }

    return QVariant();
}

QVariant pMacHelpers::toQVariant( CFBundleRef bundle )
{
    return bundle ? toQVariant( CFBundleGetIdentifier( bundle ) ) : QVariant();
}

QVariant pMacHelpers::toQVariant( CFTypeRef ref )
{
    const CFTypeID id = CFGetTypeID( ref );

    if ( id == CFStringGetTypeID() ) {
        return toQVariant( (CFStringRef)ref );
    }
    else if ( id == CFBooleanGetTypeID() ) {
        return toQVariant( (CFBooleanRef)ref );
    }
    else if ( id == CFBundleGetTypeID() ) {
        return toQVariant( (CFBundleRef)ref );
    }
    else if ( id == CFNumberGetTypeID() ) {
        return toQVariant( (CFNumberRef)ref );
    }
    else if ( id == CFDictionaryGetTypeID() ) {
        return toQVariantMap( (CFDictionaryRef)ref );
    }
    else if ( id == CFArrayGetTypeID() ) {
        return toQVariantList( (CFArrayRef)ref );
    }
    else if ( id == CFDataGetTypeID() ) {
        return toQVariant( (CFDataRef)ref );
    }
    else if ( id == CFURLGetTypeID() ) {
        return toQVariant( (CFURLRef)ref );
    }
    else if ( id == CFUUIDGetTypeID() ) {
        return toQVariant( (CFUUIDRef)ref );
    }

    qWarning() << Q_FUNC_INFO << "Unknow ID" << id;
    CFShow( ref );

    return QVariant();
}
