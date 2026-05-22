#!/usr/bin/env python3
"""
Copy Nexus dependencies from one version path to another, or upload from a local directory.

Usage:
    export NEXUS_USERNAME=your_username
    export NEXUS_PASSWORD=your_password
    python3 tools/copy_nexus_deps.py --dest 2.22 [--source current] [--dry-run]
    python3 tools/copy_nexus_deps.py --dest 2.22 --source local:./my-deps [--dry-run]

With a Nexus source, downloads all assets under client-desktop/dependencies/<source>/
and re-uploads them under client-desktop/dependencies/<dest>/.

With a local: source, walks <dir> for files at the root and within each known OS
subdirectory (windows, windows-arm64, macos, linux, linux-arm64) and uploads them
to the matching paths under client-desktop/dependencies/<dest>/.
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile

NEXUS_BASE = "https://nexus.int.windscribe.com"
REPO_NAME = "client-desktop"
REPO_PATH = "client-desktop"
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CACERT = os.path.join(SCRIPT_DIR, "cacert.pem")

OS_SUBDIRS = ["windows", "windows-arm64", "macos", "linux", "linux-arm64"]


def get_credentials():
    username = os.environ.get("NEXUS_USERNAME")
    password = os.environ.get("NEXUS_PASSWORD")
    if not username or not password:
        print("Error: Set NEXUS_USERNAME and NEXUS_PASSWORD environment variables.")
        sys.exit(1)
    return username, password


def curl(args, username, password):
    cmd = [
        "curl", "--silent", "--show-error", "--fail",
        "-u", f"{username}:{password}",
        "--cacert", CACERT,
    ] + args
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result


def search_assets_in_group(username, password, group):
    """Search for assets in a specific group (directory) using the Nexus search API."""
    assets = []
    continuation_token = None

    while True:
        url = (f"{NEXUS_BASE}/service/rest/v1/search/assets"
               f"?repository={REPO_NAME}&group={group}")
        if continuation_token:
            url += f"&continuationToken={continuation_token}"

        result = curl([url], username, password)
        if result.returncode != 0:
            return assets

        data = json.loads(result.stdout)
        assets.extend(data.get("items", []))

        continuation_token = data.get("continuationToken")
        if not continuation_token:
            break

    return assets


def list_assets(username, password, source_prefix):
    """List all assets under the source prefix by querying known subdirectories."""
    assets = []

    # Check the root directory itself
    group = f"/{source_prefix}"
    sys.stdout.write(f"  Checking {group} ...")
    sys.stdout.flush()
    found = search_assets_in_group(username, password, group)
    assets.extend(found)
    print(f" {len(found)} asset(s)")

    # Check each known OS subdirectory
    for subdir in OS_SUBDIRS:
        group = f"/{source_prefix}/{subdir}"
        sys.stdout.write(f"  Checking {group} ...")
        sys.stdout.flush()
        found = search_assets_in_group(username, password, group)
        assets.extend(found)
        print(f" {len(found)} asset(s)")

    return assets


def list_local_files(local_dir):
    """Find files in local_dir at root and within each known OS subdirectory."""
    files = []

    for name in sorted(os.listdir(local_dir)):
        full = os.path.join(local_dir, name)
        if os.path.isfile(full) and not name.startswith("."):
            files.append((name, full))

    for subdir in OS_SUBDIRS:
        sub_full = os.path.join(local_dir, subdir)
        if not os.path.isdir(sub_full):
            continue
        for name in sorted(os.listdir(sub_full)):
            full = os.path.join(sub_full, name)
            if os.path.isfile(full) and not name.startswith("."):
                files.append((f"{subdir}/{name}", full))

    return files


def upload_local_file(relative_path, local_path, username, password, dest_prefix, dry_run):
    """Upload a local file to dest_prefix/relative_path on Nexus."""
    dest_path = f"{dest_prefix}/{relative_path}"
    upload_url = f"{NEXUS_BASE}/repository/{REPO_NAME}/{dest_path}"

    if dry_run:
        print(f"  [dry-run] {local_path} -> {dest_path}")
        return True

    file_size = os.path.getsize(local_path)
    sys.stdout.write(f"  {relative_path} ({file_size:,} bytes) ... uploading")
    sys.stdout.flush()

    result = curl(["--upload-file", local_path, upload_url], username, password)
    if result.returncode != 0:
        print(f" FAILED: {result.stderr.strip()}")
        return False

    print(" OK")
    return True


def copy_asset(asset, username, password, source_prefix, dest_prefix, tmpdir, dry_run):
    """Download an asset and re-upload it with the new path."""
    source_path = asset["path"]
    relative_path = source_path[len(source_prefix):]
    dest_path = dest_prefix + relative_path

    download_url = asset["downloadUrl"]
    upload_url = f"{NEXUS_BASE}/repository/{REPO_NAME}/{dest_path}"

    local_file = os.path.join(tmpdir, os.path.basename(source_path))

    if dry_run:
        print(f"  [dry-run] {source_path} -> {dest_path}")
        return True

    sys.stdout.write(f"  {relative_path} ... downloading")
    sys.stdout.flush()

    result = curl(["-o", local_file, download_url], username, password)
    if result.returncode != 0:
        print(f" FAILED: {result.stderr.strip()}")
        return False

    file_size = os.path.getsize(local_file)
    sys.stdout.write(f" ({file_size:,} bytes) ... uploading")
    sys.stdout.flush()

    result = curl(["--upload-file", local_file, upload_url], username, password)
    if result.returncode != 0:
        print(f" FAILED: {result.stderr.strip()}")
        os.remove(local_file)
        return False

    print(" OK")
    os.remove(local_file)
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Copy Nexus dependencies between version paths, or upload from a local directory.")
    parser.add_argument("--source", default="current",
                        help="Source version (default: current), or 'local:<dir>' to upload from a local directory "
                             "mirroring the Nexus layout")
    parser.add_argument("--dest", required=True, help="Destination version (e.g. 2.22)")
    parser.add_argument("--dry-run", action="store_true", help="List what would be copied without doing it")
    args = parser.parse_args()

    dest_prefix = f"{REPO_PATH}/dependencies/{args.dest}"

    if not os.path.isfile(CACERT):
        print(f"Error: CA certificate not found at {CACERT}")
        sys.exit(1)

    username, password = get_credentials()

    if args.source.startswith("local:"):
        local_dir = args.source[len("local:"):]
        if not local_dir or not os.path.isdir(local_dir):
            print(f"Error: '{local_dir}' is not a directory")
            sys.exit(1)

        print(f"Source: {os.path.abspath(local_dir)} (local)")
        print(f"Dest:   {NEXUS_BASE}/repository/{REPO_NAME}/{dest_prefix}/")
        print(f"\nEnumerating files under {local_dir}/...")

        files = list_local_files(local_dir)

        if not files:
            print(f"\nNo files found at the root of {local_dir} or under {OS_SUBDIRS}.")
            sys.exit(1)

        print(f"\nFound {len(files)} file(s):")
        for relative, full in files:
            print(f"  {relative}  ({os.path.getsize(full):,} bytes)")

        if args.dry_run:
            print(f"\n[dry-run] Would upload {len(files)} file(s) to {dest_prefix}/")
            return

        print(f"\nUpload all {len(files)} file(s) to {dest_prefix}/? [y/N] ", end="")
        response = input().strip()
        if response.lower() != "y":
            print("Aborted.")
            sys.exit(0)

        print()
        success = 0
        failed = 0
        for relative, full in files:
            if upload_local_file(relative, full, username, password, dest_prefix, False):
                success += 1
            else:
                failed += 1

        print(f"\nDone: {success} succeeded, {failed} failed.")
        if failed > 0:
            sys.exit(1)
        return

    source_prefix = f"{REPO_PATH}/dependencies/{args.source}"

    print(f"Source: {NEXUS_BASE}/repository/{REPO_NAME}/{source_prefix}/")
    print(f"Dest:   {NEXUS_BASE}/repository/{REPO_NAME}/{dest_prefix}/")
    print(f"\nEnumerating assets under {source_prefix}/...")

    assets = list_assets(username, password, source_prefix)

    if not assets:
        print("\nNo assets found under the source path.")
        sys.exit(1)

    print(f"\nFound {len(assets)} total asset(s):")
    for a in assets:
        relative = a["path"][len(source_prefix):]
        print(f"  {relative}")

    if args.dry_run:
        print(f"\n[dry-run] Would copy {len(assets)} asset(s) to {dest_prefix}/")
        return

    print(f"\nCopy all {len(assets)} asset(s) to {dest_prefix}/? [y/N] ", end="")
    response = input().strip()
    if response.lower() != "y":
        print("Aborted.")
        sys.exit(0)

    print()
    with tempfile.TemporaryDirectory() as tmpdir:
        success = 0
        failed = 0
        for asset in assets:
            if copy_asset(asset, username, password, source_prefix, dest_prefix, tmpdir, False):
                success += 1
            else:
                failed += 1

    print(f"\nDone: {success} succeeded, {failed} failed.")
    if failed > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
