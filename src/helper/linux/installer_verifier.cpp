#include "installer_verifier.h"

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <openssl/evp.h>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "../../client/client-common/utils/executable_signature/executable_signature_defs.h"

namespace {

constexpr const char *kStagingDir = "/var/lib/windscribe/update";
constexpr const char *kGpgvPath = "/usr/bin/gpgv";

constexpr off_t kMaxPubBytes = 64 * 1024;
constexpr off_t kMaxSigBytes = 8 * 1024;
constexpr off_t kMaxPackageBytes = 128 * 1024 * 1024;

// Reasons that get tagged in spdlog::error so support can diagnose from journalctl alone.
constexpr const char *kTagSourceRejected = "source-rejected";
constexpr const char *kTagFingerprintMismatch = "fingerprint-mismatch";
constexpr const char *kTagSignatureInvalid = "signature-invalid";
constexpr const char *kTagFormatUnsupported = "format-unsupported";
constexpr const char *kTagStagingFailed = "staging-failed";

enum class Format { Unknown, Deb, Rpm, ArchPkg };

struct StagedPaths {
    std::string pkg;
    std::string pub;
    std::string detachedSig;
};

Format detectFormat(const std::string &path) {
    if (path.size() > 4 && path.compare(path.size() - 4, 4, ".deb") == 0) {
        return Format::Deb;
    }
    if (path.size() > 4 && path.compare(path.size() - 4, 4, ".rpm") == 0) {
        return Format::Rpm;
    }
    if (path.size() > 12 && path.compare(path.size() - 12, 12, ".pkg.tar.zst") == 0) {
        return Format::ArchPkg;
    }
    return Format::Unknown;
}

const char *formatExtension(Format f) {
    switch (f) {
        case Format::Deb: return ".deb";
        case Format::Rpm: return ".rpm";
        case Format::ArchPkg: return ".pkg.tar.zst";
        case Format::Unknown: return "";
    }
    return "";
}

// Extension of the detached signature shipped beside a package: .asc for .deb/.rpm (both
// go through the same gpgv path), .sig for Arch.
const char *detachedSigExtension(Format f) {
    switch (f) {
        case Format::Deb:
        case Format::Rpm: return ".asc";
        case Format::ArchPkg: return ".sig";
        case Format::Unknown: return "";
    }
    return "";
}

// Source-side hardening. Rejects symlinks, non-regular files, and packages sitting in a
// world-writable parent directory without sticky bit — both shapes a non-root attacker
// could use to swap content under the helper before the copy below.
bool validateSource(const std::string &path, off_t maxBytes) {
    struct stat st;
    if (::lstat(path.c_str(), &st) != 0) {
        spdlog::error("[{}] lstat({}) failed: {}", kTagSourceRejected, path, std::strerror(errno));
        return false;
    }
    if (!S_ISREG(st.st_mode)) {
        spdlog::error("[{}] {} is not a regular file (mode {:o})", kTagSourceRejected, path, st.st_mode);
        return false;
    }
    if (st.st_size < 0 || st.st_size > maxBytes) {
        spdlog::error("[{}] {} size {} exceeds cap {}", kTagSourceRejected, path,
                      static_cast<long long>(st.st_size), static_cast<long long>(maxBytes));
        return false;
    }
    std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (parent.empty()) {
        parent = ".";
    }
    struct stat pst;
    if (::stat(parent.c_str(), &pst) != 0) {
        spdlog::error("[{}] stat({}) failed: {}", kTagSourceRejected, parent.string(), std::strerror(errno));
        return false;
    }
    if ((pst.st_mode & S_IWOTH) && !(pst.st_mode & S_ISVTX)) {
        spdlog::error("[{}] parent dir {} is world-writable without sticky bit", kTagSourceRejected, parent.string());
        return false;
    }
    return true;
}

bool readFileBytes(const std::string &path, off_t maxBytes, std::vector<uint8_t> &out) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    struct stat st;
    if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 || st.st_size > maxBytes) {
        ::close(fd);
        return false;
    }
    out.resize(static_cast<size_t>(st.st_size));
    size_t total = 0;
    while (total < out.size()) {
        ssize_t n = ::read(fd, out.data() + total, out.size() - total);
        if (n == 0) {
            break;
        }
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            ::close(fd);
            return false;
        }
        total += static_cast<size_t>(n);
    }
    ::close(fd);
    out.resize(total);
    return true;
}

// Copy src to dst without ever following a symlink at src. O_NOFOLLOW means an attacker
// who swapped the validated source for a symlink between validateSource() and this call
// gets ELOOP rather than a redirected read. The maxBytes cap is re-checked here after
// the open so a content-swap between validateSource's lstat and our open can't smuggle
// an oversized payload past the size limit.
bool copyToStaging(const std::string &src, off_t maxBytes, const std::string &dst) {
    const int srcFd = ::open(src.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (srcFd < 0) {
        spdlog::error("[{}] open({}) failed: {}", kTagStagingFailed, src, std::strerror(errno));
        return false;
    }
    struct stat sst;
    if (::fstat(srcFd, &sst) != 0 || !S_ISREG(sst.st_mode)) {
        spdlog::error("[{}] fstat({}) rejected: not a regular file", kTagStagingFailed, src);
        ::close(srcFd);
        return false;
    }
    if (sst.st_size < 0 || sst.st_size > maxBytes) {
        spdlog::error("[{}] {} size {} exceeds cap {}", kTagStagingFailed, src,
                      static_cast<long long>(sst.st_size), static_cast<long long>(maxBytes));
        ::close(srcFd);
        return false;
    }

    const int dstFd = ::open(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (dstFd < 0) {
        spdlog::error("[{}] open({}) failed: {}", kTagStagingFailed, dst, std::strerror(errno));
        ::close(srcFd);
        return false;
    }

    char buf[64 * 1024];
    off_t copied = 0;
    for (;;) {
        ssize_t n = ::read(srcFd, buf, sizeof(buf));
        if (n == 0) {
            break;
        }
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            spdlog::error("[{}] read({}) failed: {}", kTagStagingFailed, src, std::strerror(errno));
            ::close(srcFd);
            ::close(dstFd);
            ::unlink(dst.c_str());
            return false;
        }
        copied += n;
        if (copied > maxBytes) {
            spdlog::error("[{}] {} grew past cap {} mid-copy", kTagStagingFailed, src,
                          static_cast<long long>(maxBytes));
            ::close(srcFd);
            ::close(dstFd);
            ::unlink(dst.c_str());
            return false;
        }
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = ::write(dstFd, buf + written, n - written);
            if (w < 0) {
                if (errno == EINTR) {
                    continue;
                }
                spdlog::error("[{}] write({}) failed: {}", kTagStagingFailed, dst, std::strerror(errno));
                ::close(srcFd);
                ::close(dstFd);
                ::unlink(dst.c_str());
                return false;
            }
            written += w;
        }
    }
    ::close(srcFd);
    if (::close(dstFd) != 0) {
        spdlog::error("[{}] close({}) failed: {}", kTagStagingFailed, dst, std::strerror(errno));
        ::unlink(dst.c_str());
        return false;
    }
    return true;
}

bool prepareStagingDir() {
    std::error_code ec;
    std::filesystem::remove_all(kStagingDir, ec);
    // remove_all on a non-existent dir is not an error; only fail on a real I/O error.
    if (ec) {
        spdlog::error("[{}] wiping {} failed: {}", kTagStagingFailed, kStagingDir, ec.message());
        return false;
    }
    // Ensure the parent exists (package postinst should create /var/lib/windscribe, but
    // be defensive). std::filesystem::create_directories uses the OS default mode (0755
    // minus umask) for the parent, which is fine — it's the leaf we care about.
    std::filesystem::path parent = std::filesystem::path(kStagingDir).parent_path();
    std::filesystem::create_directories(parent, ec);
    if (ec) {
        spdlog::error("[{}] mkdir parent {} failed: {}", kTagStagingFailed, parent.string(), ec.message());
        return false;
    }
    // Root-only: the staged package and its sibling .pub/.sig are written by this helper and
    // read only by the elevated install-update script — both run as root, so no non-root
    // process needs to traverse or read this directory. mkdir(0700) suffices: umask can only
    // clear permission bits, never add them, so the result is 0700 or tighter — never group-
    // or world-accessible. (The helper runs as a systemd service with UMask=0022 → 0700.)
    if (::mkdir(kStagingDir, 0700) != 0) {
        spdlog::error("[{}] mkdir {} failed: {}", kTagStagingFailed, kStagingDir, std::strerror(errno));
        return false;
    }
    return true;
}

// Strip ASCII armor (RFC 4880 §6) from a PGP key block. Skips the BEGIN/END markers,
// header lines, and the trailing CRC24 line (which starts with '='). Returns the binary
// OpenPGP packet stream on success.
bool dearmorPgp(const std::vector<uint8_t> &armored, std::vector<uint8_t> &out) {
    const std::string s(reinterpret_cast<const char *>(armored.data()), armored.size());
    // Anchor on the full marker so other armor types (SIGNATURE, MESSAGE, PRIVATE KEY BLOCK)
    // are rejected up front rather than silently mis-parsed by the packet walker downstream.
    const std::string beginMarker = "-----BEGIN PGP PUBLIC KEY BLOCK-----";
    const std::string endMarker = "-----END PGP PUBLIC KEY BLOCK-----";

    const size_t begin = s.find(beginMarker);
    if (begin == std::string::npos) {
        return false;
    }
    // Skip past the BEGIN line.
    const size_t afterBeginLine = s.find('\n', begin);
    if (afterBeginLine == std::string::npos) {
        return false;
    }
    // Skip armor headers until a blank line.
    size_t bodyStart = afterBeginLine + 1;
    while (bodyStart < s.size()) {
        const size_t eol = s.find('\n', bodyStart);
        if (eol == std::string::npos) {
            return false;
        }
        const std::string line = s.substr(bodyStart, eol - bodyStart);
        bodyStart = eol + 1;
        // A blank line (possibly with \r) marks the end of headers.
        if (line.empty() || line == "\r") {
            break;
        }
    }

    const size_t end = s.find(endMarker, bodyStart);
    if (end == std::string::npos) {
        return false;
    }

    // A .pub with a second armor block past the first BEGIN/END would be ignored by this
    // walker but would still be imported into the system rpmdb by `rpm --import`, which
    // honors every armor block in the file. Reject the whole input so downstream consumers
    // of the on-disk staged.pub see exactly the same single transferable key that gpgv
    // verified against.
    if (s.find(beginMarker, end + endMarker.size()) != std::string::npos) {
        return false;
    }

    // Concatenate base64 lines between bodyStart and end, dropping the CRC24 line ('=...').
    std::string b64;
    b64.reserve(end - bodyStart);
    size_t p = bodyStart;
    while (p < end) {
        const size_t eol = s.find('\n', p);
        const size_t lineEnd = (eol == std::string::npos || eol > end) ? end : eol;
        std::string line = s.substr(p, lineEnd - p);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        p = lineEnd + 1;
        if (line.empty()) {
            continue;
        }
        if (line[0] == '=') {
            // CRC24 line; we don't verify it (not required for security).
            continue;
        }
        b64.append(line);
    }

    if (b64.empty() || b64.size() % 4 != 0 || b64.size() > static_cast<size_t>(INT_MAX)) {
        return false;
    }
    out.resize(b64.size());
    int decoded = EVP_DecodeBlock(out.data(),
                                  reinterpret_cast<const uint8_t *>(b64.data()),
                                  static_cast<int>(b64.size()));
    if (decoded < 0) {
        return false;
    }
    // EVP_DecodeBlock pads with NULs for trailing '=' chars in the input; trim them.
    size_t pad = 0;
    while (pad < 2 && pad < b64.size() && b64[b64.size() - 1 - pad] == '=') {
        ++pad;
    }
    if (static_cast<size_t>(decoded) >= pad) {
        out.resize(static_cast<size_t>(decoded) - pad);
    } else {
        out.resize(static_cast<size_t>(decoded));
    }
    return true;
}

// OpenPGP packet header decode (RFC 4880 §4.2). Returns the tag and body length and sets
// bodyOffset to the start of the body relative to packetStart. Returns false on malformed
// or partial-length packets (we don't need to support partial-length for a primary key).
bool readPacketHeader(const std::vector<uint8_t> &data, size_t packetStart,
                      uint8_t &tag, size_t &bodyOffset, size_t &bodyLen) {
    if (packetStart >= data.size()) {
        return false;
    }
    const uint8_t h = data[packetStart];
    if ((h & 0x80) == 0) {
        return false;  // bit 7 must be set
    }
    if (h & 0x40) {
        // New-format packet.
        tag = h & 0x3F;
        if (packetStart + 1 >= data.size()) {
            return false;
        }
        const uint8_t l0 = data[packetStart + 1];
        if (l0 < 192) {
            bodyLen = l0;
            bodyOffset = packetStart + 2;
        } else if (l0 < 224) {
            if (packetStart + 2 >= data.size()) {
                return false;
            }
            bodyLen = ((l0 - 192) << 8) + data[packetStart + 2] + 192;
            bodyOffset = packetStart + 3;
        } else if (l0 == 255) {
            if (packetStart + 5 >= data.size()) {
                return false;
            }
            bodyLen = (static_cast<size_t>(data[packetStart + 2]) << 24) |
                      (static_cast<size_t>(data[packetStart + 3]) << 16) |
                      (static_cast<size_t>(data[packetStart + 4]) << 8) |
                      static_cast<size_t>(data[packetStart + 5]);
            bodyOffset = packetStart + 6;
        } else {
            // Partial-length packet — would only appear for streamed data, not a key.
            return false;
        }
    } else {
        // Old-format packet.
        tag = (h >> 2) & 0x0F;
        const uint8_t lenType = h & 0x03;
        switch (lenType) {
            case 0:
                if (packetStart + 1 >= data.size()) {
                    return false;
                }
                bodyLen = data[packetStart + 1];
                bodyOffset = packetStart + 2;
                break;
            case 1:
                if (packetStart + 2 >= data.size()) {
                    return false;
                }
                bodyLen = (static_cast<size_t>(data[packetStart + 1]) << 8) |
                          static_cast<size_t>(data[packetStart + 2]);
                bodyOffset = packetStart + 3;
                break;
            case 2:
                if (packetStart + 4 >= data.size()) {
                    return false;
                }
                bodyLen = (static_cast<size_t>(data[packetStart + 1]) << 24) |
                          (static_cast<size_t>(data[packetStart + 2]) << 16) |
                          (static_cast<size_t>(data[packetStart + 3]) << 8) |
                          static_cast<size_t>(data[packetStart + 4]);
                bodyOffset = packetStart + 5;
                break;
            default:
                return false;
        }
    }
    if (bodyOffset + bodyLen > data.size()) {
        return false;
    }
    return true;
}

// Returns the V4 fingerprint of the primary key, but ONLY if the input is exactly one
// transferable public key. Enforces three invariants that together close the multi-key
// smuggling attack: (a) the very first packet must be a V4 Public-Key (tag 6), so the
// fingerprint we trust corresponds to the primary key RFC 4880 §11.1 says must lead a
// transferable key; (b) no additional Public-Key packets may appear later in the stream,
// so an attacker cannot concat `<trusted_master><attacker_master>` and have gpgv load both
// from the same keyring; (c) only V4 fingerprints are supported here (V5/V6 use a
// different fingerprint construction and trust anchor format).
//
// The caller may pass the untouched input binary to gpgv on success: the single-tag-6
// check above guarantees there's nothing extra in the buffer.
bool extractMasterFingerprintV4(const std::vector<uint8_t> &binary, std::string &fprHexOut) {
    if (binary.empty()) {
        return false;
    }

    uint8_t firstTag = 0;
    size_t firstBodyOffset = 0;
    size_t firstBodyLen = 0;
    if (!readPacketHeader(binary, 0, firstTag, firstBodyOffset, firstBodyLen)) {
        return false;
    }
    // Tag 6 = Public-Key packet (the primary key, always first in a transferable key).
    if (firstTag != 6) {
        return false;
    }
    // Only V4 is supported; V5/V6 would have different fingerprint construction.
    if (firstBodyLen == 0 || binary[firstBodyOffset] != 4) {
        return false;
    }
    // The V4 fingerprint prefix below encodes the body length in two bytes, so a V4
    // fingerprint is only defined for a body that fits in 0xFFFF. A larger body would
    // truncate the prefix while the full body is hashed, yielding a meaningless digest.
    if (firstBodyLen > 0xFFFF) {
        return false;
    }

    // Walk the rest of the stream. Any subsequent tag-6 packet means the .pub contains a
    // second transferable key — reject the whole input so gpgv never sees the extra key.
    size_t pos = firstBodyOffset + firstBodyLen;
    while (pos < binary.size()) {
        uint8_t tag = 0;
        size_t bodyOffset = 0;
        size_t bodyLen = 0;
        if (!readPacketHeader(binary, pos, tag, bodyOffset, bodyLen)) {
            return false;
        }
        if (tag == 6) {
            return false;
        }
        pos = bodyOffset + bodyLen;
    }

    // V4 fingerprint per RFC 4880 §12.2: SHA-1 over (0x99 || uint16_be(body_len) || body)
    // of the primary public-key packet. Returns 40 uppercase hex chars.
    unsigned char prefix[3];
    prefix[0] = 0x99;
    prefix[1] = static_cast<unsigned char>((firstBodyLen >> 8) & 0xFF);
    prefix[2] = static_cast<unsigned char>(firstBodyLen & 0xFF);

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return false;
    }
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    const bool digestOk =
        EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr) == 1 &&
        EVP_DigestUpdate(ctx, prefix, sizeof(prefix)) == 1 &&
        EVP_DigestUpdate(ctx, &binary[firstBodyOffset], firstBodyLen) == 1 &&
        EVP_DigestFinal_ex(ctx, digest, &digestLen) == 1;
    EVP_MD_CTX_free(ctx);
    if (!digestOk || digestLen == 0) {
        return false;
    }

    static const char hex[] = "0123456789ABCDEF";
    fprHexOut.clear();
    fprHexOut.reserve(digestLen * 2);
    for (unsigned int i = 0; i < digestLen; ++i) {
        fprHexOut.push_back(hex[(digest[i] >> 4) & 0xF]);
        fprHexOut.push_back(hex[digest[i] & 0xF]);
    }
    return true;
}

bool fingerprintIsTrusted(const std::string &fpr) {
    static const char *const accepted[] = LINUX_SIGNING_MASTER_FINGERPRINTS;
    for (const char *want : accepted) {
        if (fpr == want) {
            return true;
        }
    }
    return false;
}

// Run a child process synchronously with a fully-controlled argv (no shell, no env leak).
// Returns true iff the child exits cleanly with status 0. Stderr/stdout inherit from the
// parent so the rpm/gpgv output (BADSIG / NOKEY / etc.) lands in journald.
bool runChild(const char *exePath, const std::vector<std::string> &args, const char *failTag) {
    // Build argv in the parent before fork() so the child reads the pointers from the
    // COW'd address space — post-fork malloc could deadlock on glibc's arena lock if any
    // sibling thread held it at fork() time. The helper is single-threaded today, but
    // this hardening survives future thread additions.
    std::vector<char *> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char *>(exePath));
    for (const auto &a : args) {
        argv.push_back(const_cast<char *>(a.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = ::fork();
    if (pid < 0) {
        spdlog::error("[{}] fork failed: {}", failTag, std::strerror(errno));
        return false;
    }
    if (pid == 0) {
        const int devnull = ::open("/dev/null", O_RDONLY);
        if (devnull < 0) {
            ::_exit(127);
        }
        if (::dup2(devnull, STDIN_FILENO) < 0) {
            ::_exit(127);
        }
        if (devnull != STDIN_FILENO) {
            ::close(devnull);
        }
        // Close every other inherited fd so gpgv can't pin the helper's IPC socket or log
        // file if it hangs / gets SIGSTOP'd. close_range is a single async-signal-safe
        // syscall (Linux 5.9+; our minimum supported kernel is 5.15, so it is always
        // present) that closes everything in [3, ~0). If it ever fails we refuse to exec
        // rather than let gpgv inherit the helper's fds: log via bare write() — spdlog is
        // not async-signal-safe in a post-fork child — then _exit so the parent's waitpid
        // sees failure and verification aborts.
        if (::syscall(SYS_close_range, 3U, ~0U, 0U) < 0) {
            static const char msg[] = "runChild: close_range failed; refusing to exec\n";
            ssize_t n = ::write(STDERR_FILENO, msg, sizeof(msg) - 1);
            (void)n;
            ::_exit(127);
        }
        ::execv(exePath, argv.data());
        ::_exit(127);
    }

    int status = 0;
    while (::waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            spdlog::error("[{}] waitpid failed: {}", failTag, std::strerror(errno));
            return false;
        }
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        spdlog::error("[{}] {} exit status {} (raw {})",
                      failTag, exePath, WIFEXITED(status) ? WEXITSTATUS(status) : -1, status);
        return false;
    }
    return true;
}

bool verifyDetachedGpg(const std::string &dataPath, const std::string &sigPath,
                      const std::vector<uint8_t> &keyBinary) {
    char tmpl[] = "/tmp/windscribe-verify-XXXXXX";
    const char *dir = ::mkdtemp(tmpl);
    if (!dir) {
        spdlog::error("[{}] mkdtemp failed: {}", kTagSignatureInvalid, std::strerror(errno));
        return false;
    }
    const std::string keyringPath = std::string(dir) + "/keyring";

    bool ok = false;
    {
        std::ofstream f(keyringPath, std::ios::binary | std::ios::trunc);
        if (f) {
            f.write(reinterpret_cast<const char *>(keyBinary.data()),
                    static_cast<std::streamsize>(keyBinary.size()));
            f.close();
            if (::chmod(keyringPath.c_str(), 0600) != 0) {
                spdlog::warn("chmod({}, 0600) failed: {}", keyringPath, std::strerror(errno));
            }
            ok = runChild(kGpgvPath,
                          {"--keyring", keyringPath, sigPath, dataPath},
                          kTagSignatureInvalid);
        } else {
            spdlog::error("[{}] failed to write keyring at {}", kTagSignatureInvalid, keyringPath);
        }
    }

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    return ok;
}

void wipeStaging() {
    std::error_code ec;
    std::filesystem::remove_all(kStagingDir, ec);
}

}  // namespace

namespace InstallerVerifier {

std::optional<std::string> stageAndVerify(const std::string &srcPath) {
    if (!validateSource(srcPath, kMaxPackageBytes)) {
        return std::nullopt;
    }

    const Format fmt = detectFormat(srcPath);
    if (fmt == Format::Unknown) {
        spdlog::error("[{}] unrecognized package extension: {}", kTagFormatUnsupported, srcPath);
        return std::nullopt;
    }

    // Build sibling source paths and require their presence on disk before staging. Every
    // supported format ships a detached signature alongside the package (extension per
    // detachedSigExtension); the helper verifies that detached form with gpgv and is the
    // sole authenticity gate — the package managers are told to skip their own checks.
    const std::string srcPub = srcPath + ".pub";
    const std::string srcDetachedSig = srcPath + detachedSigExtension(fmt);
    if (!validateSource(srcPub, kMaxPubBytes)) {
        return std::nullopt;
    }
    if (!srcDetachedSig.empty() && !validateSource(srcDetachedSig, kMaxSigBytes)) {
        return std::nullopt;
    }

    if (!prepareStagingDir()) {
        return std::nullopt;
    }

    StagedPaths staged;
    staged.pkg = std::string(kStagingDir) + "/installer" + formatExtension(fmt);
    staged.pub = staged.pkg + ".pub";
    staged.detachedSig = staged.pkg + detachedSigExtension(fmt);

    if (!copyToStaging(srcPath, kMaxPackageBytes, staged.pkg)) {
        wipeStaging();
        return std::nullopt;
    }
    if (!copyToStaging(srcPub, kMaxPubBytes, staged.pub)) {
        wipeStaging();
        return std::nullopt;
    }
    if (!staged.detachedSig.empty() && !copyToStaging(srcDetachedSig, kMaxSigBytes, staged.detachedSig)) {
        wipeStaging();
        return std::nullopt;
    }

    // Fingerprint check on the staged .pub. This is what makes the freshly-downloaded key
    // trustworthy — without this, an attacker who can serve the package can also swap the
    // public key. The compile-time fingerprint is the anchor of the entire trust chain.
    std::vector<uint8_t> pubArmored;
    if (!readFileBytes(staged.pub, kMaxPubBytes, pubArmored)) {
        spdlog::error("[{}] failed to read staged pub at {}", kTagFingerprintMismatch, staged.pub);
        wipeStaging();
        return std::nullopt;
    }
    std::vector<uint8_t> pubBinary;
    if (!dearmorPgp(pubArmored, pubBinary)) {
        spdlog::error("[{}] dearmor failed for {}", kTagFingerprintMismatch, staged.pub);
        wipeStaging();
        return std::nullopt;
    }
    std::string fpr;
    if (!extractMasterFingerprintV4(pubBinary, fpr)) {
        spdlog::error("[{}] .pub at {} did not contain a single V4 transferable public key starting with the primary",
                      kTagFingerprintMismatch, staged.pub);
        wipeStaging();
        return std::nullopt;
    }
    if (!fingerprintIsTrusted(fpr)) {
        spdlog::error("[{}] downloaded key fingerprint {} is not in the trusted set", kTagFingerprintMismatch, fpr);
        wipeStaging();
        return std::nullopt;
    }

    bool sigOk = false;
    switch (fmt) {
        case Format::Deb:
        case Format::Rpm:
        case Format::ArchPkg:
            sigOk = verifyDetachedGpg(staged.pkg, staged.detachedSig, pubBinary);
            break;
        case Format::Unknown:
            break;
    }
    if (!sigOk) {
        wipeStaging();
        return std::nullopt;
    }

    return staged.pkg;
}

void cleanupStaged() {
    wipeStaging();
}

}  // namespace InstallerVerifier
