#!/bin/bash
# Sign every .deb and .rpm in $2 with the GPG key id $1, emitting a detached
# .asc and a key export .pub alongside each. Used by the sign-linux-packages
# CMake target; invoked once per build_all run when --sign-app is set.
#
# Idempotent: re-signing an already-signed package replaces the existing
# signature with a fresh one of the same key. That lets the same script
# serve both the combined workflow
#     ./build_all --build-deb --sign-app
# and the Windows-style split
#     ./build_all --build-deb
#     ./build_all --sign-app
#
# Requires GNUPGHOME to be configured (with the signing subkey imported and
# pinentry-mode loopback) before this runs. The CMake target sets neither —
# that's the caller's job (.gitlab-ci.yml in CI, local dev's gpg setup
# otherwise). The signing key must be passphraseless.

set -eu

if [ $# -ne 2 ]; then
    echo "usage: $0 <gpg-key-fingerprint> <build-exe-dir>" >&2
    exit 1
fi

key_id=$(printf '%s' "$1" | tr -d '[:space:]')
build_exe_dir="$2"

if [ -z "$key_id" ]; then
    echo "$0: no key id supplied" >&2
    exit 1
fi
if ! printf '%s' "$key_id" | grep -qE '^[0-9a-fA-F]{40}!?$'; then
    echo "$0: key id must be a 40-char hex fingerprint (got: $key_id)" >&2
    exit 1
fi
if [ ! -d "$build_exe_dir" ]; then
    echo "$0: $build_exe_dir not found" >&2
    exit 1
fi

shopt -s nullglob

# rpmsign uses the RPM macro %__gpg to locate the gpg binary. Its default
# (/usr/bin/gpg on Fedora, /usr/bin/gpg2 on some Debian rpm packages) may not
# match where gpg actually lives on the CI runner. Pin it to whatever PATH
# gives us so rpmsign finds the same gpg we're using everywhere else.
gpg_path=$(command -v gpg)
if [ -z "$gpg_path" ]; then
    echo "$0: gpg not found on PATH" >&2
    exit 1
fi

signed_any=0
for f in "$build_exe_dir"/*.deb "$build_exe_dir"/*.rpm; do
    echo "[sign-linux-packages] $f"
    case "$f" in
        *.rpm)
            rpmsign --define "__gpg $gpg_path" \
                    --define "_gpg_name $key_id" \
                    --addsign "$f"
            ;;
    esac
    gpg --batch --yes --detach-sign --armor --local-user "$key_id" -o "$f.asc" "$f"
    # Strip any trailing '!' for the export: 'FPR!' exports only that subkey, producing a
    # .pub with no primary key that the helper's fingerprint check would reject. The '!' is
    # kept on --local-user above, where it legitimately selects an exact signing (sub)key.
    gpg --batch --yes --armor --output "$f.pub" --export "${key_id%!}"
    signed_any=1
done

if [ "$signed_any" -eq 0 ]; then
    echo "$0: no .deb or .rpm files found in $build_exe_dir; nothing to sign" >&2
    exit 1
fi
