#include "texthighlighter.h"

#include <QTextBlock>
#include <QTextDocument>

// static
void TextHighlighter::processDocument(QTextDocument *doc, bool do_highlight, LogDataType type)
{
    Q_ASSERT(type < NUM_LOG_TYPES || type == LOG_TYPE_MIXED);

    const QBrush kBlockBrushes[NUM_LOG_TYPES] = {
        QBrush(QColor(Qt::cyan).lighter(180)),
        QBrush(QColor(Qt::yellow).lighter(180)),
        QBrush(QColor(Qt::magenta).lighter(180)),
    };

    auto block = doc->begin();
    if (block == doc->end())
        return;
    for (; block != doc->end(); block = block.next()) {
        auto blockFormat = block.blockFormat();
        if (block.text().isEmpty())
            continue;
        const auto first_char = block.text()[0].toLatin1();
        if (do_highlight && first_char != '=') {
            if (type == LOG_TYPE_MIXED) {
                switch (first_char) {
                case 'G':
                    blockFormat.setBackground(kBlockBrushes[LOG_TYPE_GUI]);
                    break;
                case 'E':
                    blockFormat.setBackground(kBlockBrushes[LOG_TYPE_ENGINE]);
                    break;
                case 'S':
                    blockFormat.setBackground(kBlockBrushes[LOG_TYPE_SERVICE]);
                    break;
                default:
                    break;
                }
            } else {
                blockFormat.setBackground(kBlockBrushes[type]);
            }
        } else {
            blockFormat.setBackground(QBrush());
        }
        QTextCursor(block).setBlockFormat(blockFormat);
    }
}
