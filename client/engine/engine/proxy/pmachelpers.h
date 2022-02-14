#ifndef PMACHELPERS_H
#define PMACHELPERS_H

#include <QVariant>

#import <CoreFoundation/CFDictionary.h>
#import <CoreFoundation/CFArray.h>
#import <CoreFoundation/CFString.h>
#import <CoreFoundation/CFNumber.h>
#import <CoreFoundation/CFData.h>
#import <CoreFoundation/CFURL.h>
#import <CoreFoundation/CFUUID.h>
#import <CoreFoundation/CFBundle.h>
#import <CoreFoundation/CFBase.h>

namespace pMacHelpers
{
    QVariantMap toQVariantMap( CFDictionaryRef dict );
    QVariantList toQVariantList( CFArrayRef array );
    QVariant toQVariant( CFStringRef string );
    QVariant toQVariant( CFBooleanRef value );
    QVariant toQVariant( CFNumberRef number );
    QVariant toQVariant( CFDataRef data );
    QVariant toQVariant( CFURLRef url );
    QVariant toQVariant( CFUUIDRef uuid );
    QVariant toQVariant( CFBundleRef bundle );
    QVariant toQVariant( CFTypeRef ref );
};

#endif // PMACHELPERS_H
