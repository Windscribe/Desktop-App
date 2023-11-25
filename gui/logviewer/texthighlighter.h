#pragma once

#include "common.h"

class QTextBlock;
class QTextDocument;

class TextHighlighter
{
public:
    TextHighlighter(bool do_highlight, int current_filter_match_line, LogDataType type);
    void processDocument(QTextDocument *doc, const QList<int> &lines) const;

private:
    enum BlockColorType {
        COLOR_TYPE_NONE,
        COLOR_TYPE_GUI,
        COLOR_TYPE_ENGINE,
        COLOR_TYPE_SERVICE,
        COLOR_TYPE_MATCH,
        COLOR_TYPE_CURRENT,
        NUM_COLOR_TYPES
    };
    BlockColorType getBlockType(const QTextBlock &block, int block_number) const;

    bool do_highlight_;
    int current_filter_match_line_;
    LogDataType log_data_type_;
};
