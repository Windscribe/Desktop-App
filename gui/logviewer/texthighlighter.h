#ifndef TEXTHIGHLIGHTER_H
#define TEXTHIGHLIGHTER_H

#include "common.h"

class QTextDocument;

class TextHighlighter
{
public:
    static void processDocument(QTextDocument *doc, bool do_highlight, LogDataType type);
};

#endif  // TEXTHIGHLIGHTER_H
