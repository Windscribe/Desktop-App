#include "texthighlighter.h"

#include <QTextBlock>
#include <QTextDocument>

TextHighlighter::TextHighlighter(bool do_highlight, int current_filter_match_line, LogDataType type)
    : do_highlight_(do_highlight),
      current_filter_match_line_(current_filter_match_line),
      log_data_type_(type)
{
}

void TextHighlighter::processDocument(QTextDocument *doc, const QList<int> &lines) const
{
    static const QBrush kColorBrushes[NUM_COLOR_TYPES] = {
        QBrush(),
        QBrush(QColor(Qt::cyan).lighter(180)),
        QBrush(QColor(Qt::yellow).lighter(180)),
        QBrush(QColor(Qt::magenta).lighter(180)),
        QBrush(QColor(Qt::red).lighter(180)),
        QBrush(QColor(Qt::red).lighter(150))
    };

    if (!lines.isEmpty()) {
        // Fast path: process certain lines only.
        for (const int lineNumber : lines) {
            auto block = doc->findBlockByNumber(lineNumber);
            if (block == doc->end())
                continue;
            QTextCursor cursor{ block };
            BlockColorType current_color_type{ getBlockType(block, lineNumber) };
            QTextBlockFormat current_block_format{ block.blockFormat() };
            current_block_format.setBackground(kColorBrushes[current_color_type]);
            cursor.setBlockFormat(current_block_format);
        }
        return;
    }

    auto block = doc->begin();
    if (block == doc->end())
        return;

    QTextCursor cursor{ block };
    QTextBlockFormat current_block_format{ block.blockFormat() };
    BlockColorType current_color_type{ getBlockType(block, 0) };
    int blockSequence = 0;
    block = block.next();
    for (int blockNum = 1; block != doc->end(); block = block.next(), ++blockNum) {
        const auto color_type = getBlockType(block, blockNum);
        if (current_color_type == color_type) {
            ++blockSequence;
        } else {
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, blockSequence);
            current_block_format.setBackground(kColorBrushes[current_color_type]);
            cursor.setBlockFormat(current_block_format);
            current_color_type = color_type;
            cursor.movePosition(QTextCursor::NextBlock);
            current_block_format = block.blockFormat();
            blockSequence = 0;
        }
    }
    if (blockSequence > 0)
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, blockSequence);
    current_block_format.setBackground(kColorBrushes[current_color_type]);
    cursor.setBlockFormat(current_block_format);
}

TextHighlighter::BlockColorType TextHighlighter::getBlockType(
    const QTextBlock &block, int block_number) const
{
    if (block_number == current_filter_match_line_)
        return COLOR_TYPE_CURRENT;
    if (!block.text().isEmpty()) {
        const QString blockText = block.text();
        const auto first_char = blockText[0].toLatin1();
        if (first_char == '*' ||
            (log_data_type_ == LOG_TYPE_MIXED && blockText[1].toLatin1() == '*'))
            return COLOR_TYPE_MATCH;
        if (do_highlight_ && first_char != '=') {
            if (log_data_type_ == LOG_TYPE_MIXED) {
                switch (first_char) {
                case 'G': return COLOR_TYPE_GUI;
                case 'E': return COLOR_TYPE_ENGINE;
                case 'S': return COLOR_TYPE_SERVICE;
                default: break;
                }
            } else {
                switch (log_data_type_) {
                case LOG_TYPE_GUI: return COLOR_TYPE_GUI;
                case LOG_TYPE_ENGINE: return COLOR_TYPE_ENGINE;
                case LOG_TYPE_SERVICE: return COLOR_TYPE_SERVICE;
                default: break;
                }
            }
        }
    }
    return COLOR_TYPE_NONE;
}
