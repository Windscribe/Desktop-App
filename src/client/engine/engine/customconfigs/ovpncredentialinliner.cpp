#include "ovpncredentialinliner.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include "utils/log/categories.h"

namespace customconfigs {

namespace {

constexpr qint64 kMaxInlineFileSize = 1 * 1024 * 1024;

const QSet<QString> &fileBearingDirectives()
{
    static const QSet<QString> dirs = {
        "ca", "cert", "key", "pkcs12",
        "tls-auth", "tls-crypt", "tls-crypt-v2",
    };
    return dirs;
}

QString stripDirectivePrefix(const QString &line)
{
    QString s = line.trimmed();
    if (s.startsWith(QLatin1String("--"))) {
        s = s.mid(2);
    }
    return s;
}

QString extractDirectiveName(const QString &line)
{
    QString s = stripDirectivePrefix(line);
    static const QRegularExpression ws(QStringLiteral("\\s"));
    int wsAt = s.indexOf(ws);
    return (wsAt < 0 ? s : s.left(wsAt)).toLower();
}

QStringList tokenizeArgs(const QString &line)
{
    QString s = stripDirectivePrefix(line);
    static const QRegularExpression ws(QStringLiteral("\\s"));
    int wsAt = s.indexOf(ws);
    if (wsAt < 0) {
        return {};
    }
    QString argsStr = s.mid(wsAt + 1);

    QStringList args;
    QString current;
    QChar quoteChar;
    bool inQuotes = false;

    for (QChar c : argsStr) {
        if (!inQuotes && (c == '#' || c == ';')) {
            break;
        }
        if (!inQuotes && (c == '"' || c == '\'')) {
            inQuotes = true;
            quoteChar = c;
            continue;
        }
        if (inQuotes && c == quoteChar) {
            inQuotes = false;
            continue;
        }
        if (!inQuotes && c.isSpace()) {
            if (!current.isEmpty()) {
                args << current;
                current.clear();
            }
            continue;
        }
        current += c;
    }
    if (!current.isEmpty()) {
        args << current;
    }
    return args;
}

bool containsNulOrControl(const QString &s)
{
    for (QChar c : s) {
        if (c.unicode() < 0x20 && c != '\t') {
            return true;
        }
    }
    return false;
}

// On failure, errorReason is set to the underlying reason only (no path); the
// caller composes the user-facing message using the path token the user wrote.
bool readFile(const QString &path, QByteArray &out, QString &errorReason)
{
    // Reject character devices, FIFOs, sockets, etc. up front. They would otherwise
    // satisfy QFile::open() but yield unbounded reads from things like /dev/zero or
    // blocking reads from empty FIFOs.
    if (!QFileInfo(path).isFile()) {
        errorReason = "not a regular file";
        return false;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        errorReason = f.errorString();
        return false;
    }

    // Bounded read: QFile::size() can lie for special files that QFileInfo::isFile()
    // considers regular (procfs entries report size 0 but have content). Read one
    // byte past the cap so we can detect oversize without trusting size().
    out = f.read(kMaxInlineFileSize + 1);
    if (out.size() > kMaxInlineFileSize) {
        errorReason = QString("file is larger than the %1-byte inlining limit").arg(kMaxInlineFileSize);
        return false;
    }
    return true;
}

// Wrap base64 at 64 columns (PEM convention). OpenVPN feeds inline pkcs12
// content through OpenSSL's BIO_f_base64 in its default mode (no
// BIO_FLAGS_BASE64_NO_NL), which requires the input to be line-wrapped — a
// single long line gets rejected. If a future OpenVPN release ever flips
// that flag, this wrap can become a single line.
QByteArray base64Wrap(const QByteArray &data)
{
    QByteArray b64 = data.toBase64();
    QByteArray wrapped;
    wrapped.reserve(b64.size() + b64.size() / 64 + 1);
    for (int i = 0; i < b64.size(); i += 64) {
        wrapped.append(b64.mid(i, 64));
        wrapped.append('\n');
    }
    return wrapped;
}

bool isInlineBlockTag(const QString &trimmed, QString &tagName, bool &isClosing)
{
    if (!trimmed.startsWith('<') || !trimmed.endsWith('>')) {
        return false;
    }
    QString inner = trimmed.mid(1, trimmed.size() - 2).trimmed();
    if (inner.isEmpty()) {
        return false;
    }
    if (inner.startsWith('/')) {
        isClosing = true;
        tagName = inner.mid(1).toLower();
    } else {
        isClosing = false;
        tagName = inner.toLower();
    }
    return !tagName.isEmpty();
}

// PEM-encoded certs/keys and OpenVPN static keys only contain base64/hex
// payload between -----BEGIN-----/-----END----- markers — no angle brackets.
// Any '<' or '>' in the file content is therefore either malformed input or
// a deliberate attempt to smuggle inline-block tags through into the helper-
// bound config. pkcs12 is exempt: we base64-encode it, so any embedded
// brackets are opaque by the time the helper parses the config.
bool containsAngleBracket(const QByteArray &content)
{
    return content.contains('<') || content.contains('>');
}

} // namespace

OvpnCredentialInliner::Result OvpnCredentialInliner::process(const QString &config, const QString &resolutionBaseDir)
{
    Result result;
    result.success = true;

    QDir baseDir(resolutionBaseDir);
    QString output;
    bool insideInlineBlock = false;

    QStringList lines = config.split('\n');
    if (!lines.isEmpty() && lines.last().isEmpty()) {
        lines.removeLast();
    }

    for (const QString &raw : lines) {
        QString trimmed = raw.trimmed();

        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';')) {
            output += raw;
            output += '\n';
            continue;
        }

        QString tagName;
        bool isClosing = false;
        if (isInlineBlockTag(trimmed, tagName, isClosing)) {
            insideInlineBlock = !isClosing;
            output += raw;
            output += '\n';
            continue;
        }

        if (insideInlineBlock) {
            output += raw;
            output += '\n';
            continue;
        }

        QString name = extractDirectiveName(raw);
        if (!fileBearingDirectives().contains(name)) {
            output += raw;
            output += '\n';
            continue;
        }

        QStringList args = tokenizeArgs(raw);
        if (args.isEmpty()) {
            // Bare form has no argument to inline. Drop it — OpenVPN treats
            // these directives as requiring a file argument, and the helper
            // now rejects the directive name regardless of arguments.
            qDebug(LOG_CUSTOM_OVPN) << "Dropping bare directive with no file argument:" << name;
            continue;
        }

        const QString &filePath = args[0];
        if (containsNulOrControl(filePath)) {
            result.success = false;
            result.errorMessage = QString("Directive '%1' has an invalid path argument").arg(name);
            return result;
        }

        QString resolved = QFileInfo(filePath).isAbsolute()
            ? filePath
            : baseDir.absoluteFilePath(filePath);

        QByteArray bytes;
        QString readReason;
        if (!readFile(resolved, bytes, readReason)) {
            result.success = false;
            result.errorMessage = QString("Directive '%1': could not read '%2': %3").arg(name, filePath, readReason);
            return result;
        }

        if (name != QLatin1String("pkcs12") && containsAngleBracket(bytes)) {
            result.success = false;
            result.errorMessage = QString("Directive '%1': '%2' is not a valid PEM or static-key file (contains '<' or '>')").arg(name, filePath);
            return result;
        }

        output += '<';
        output += name;
        output += ">\n";
        if (name == QLatin1String("pkcs12")) {
            output += QString::fromLatin1(base64Wrap(bytes));
        } else {
            output += QString::fromUtf8(bytes);
            if (!bytes.endsWith('\n')) {
                output += '\n';
            }
        }
        output += "</";
        output += name;
        output += ">\n";

        if (name == QLatin1String("tls-auth") && args.size() >= 2) {
            output += "key-direction ";
            output += args[1];
            output += '\n';
        } else if (name == QLatin1String("tls-crypt-v2") && args.size() >= 2) {
            qDebug(LOG_CUSTOM_OVPN) << "Dropping tls-crypt-v2 trailing argument; the flag is server-only and cannot ride along with an inline block:" << args[1];
        }
    }

    result.config = output;
    return result;
}

} // namespace customconfigs
