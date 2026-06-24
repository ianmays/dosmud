#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 <linux|windows> <tag> <output-dir>" >&2
    exit 1
fi

platform=$1
tag=$2
out_dir=$3
version_header=build/include/version.h
out_dir_abs=$(cd "$out_dir" 2>/dev/null && pwd || true)

if [ ! -f "$version_header" ]; then
    echo "missing $version_header - build the release binary first" >&2
    exit 1
fi

base_version=$(sed -n 's/^#define BUILD_BASE_VERSION "\(.*\)"/\1/p' "$version_header")
tag_version=${tag#v}

if [ -z "$base_version" ]; then
    echo "could not read BUILD_BASE_VERSION from $version_header" >&2
    exit 1
fi

if [ "$tag_version" != "$base_version" ]; then
    echo "tag $tag does not match BUILD_BASE_VERSION $base_version" >&2
    exit 1
fi

version_string=$(sed -n 's/^#define BUILD_VERSION_STRING "\(.*\)"/\1/p' "$version_header")
if [ -z "$version_string" ]; then
    echo "could not read BUILD_VERSION_STRING from $version_header" >&2
    exit 1
fi

case "$platform" in
    linux)
        bin_name=dosmud
        archive_name="dosmud-${tag}-linux-x86_64.tar.gz"
        package_dir="dosmud-${tag}-linux-x86_64"
        ;;
    windows)
        bin_name=dosmud.exe
        archive_name="dosmud-${tag}-windows-x86_64.zip"
        package_dir="dosmud-${tag}-windows-x86_64"
        ;;
    *)
        echo "unsupported platform: $platform" >&2
        exit 1
        ;;
esac

if [ ! -f "$bin_name" ]; then
    echo "missing $bin_name - build the $platform binary first" >&2
    exit 1
fi

tmp_root=$(mktemp -d)
pkg_root="$tmp_root/$package_dir"

mkdir -p "$pkg_root"
mkdir -p "$out_dir"

if [ -z "$out_dir_abs" ]; then
    out_dir_abs=$(cd "$out_dir" && pwd)
fi

cp "$bin_name" "$pkg_root/"
cp README.md VERSION "$pkg_root/"

cat > "$pkg_root/release-metadata.txt" <<EOF
tag: $tag
platform: $platform
build_version: $version_string
artifact: $archive_name
EOF

case "$platform" in
    linux)
        tar -czf "$out_dir/$archive_name" -C "$tmp_root" "$package_dir"
        ;;
    windows)
        python3 - "$tmp_root" "$package_dir" "$out_dir_abs/$archive_name" <<'PY'
import os
import sys
import zipfile

root = sys.argv[1]
pkg_dir = sys.argv[2]
archive_path = sys.argv[3]
pkg_root = os.path.join(root, pkg_dir)

with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
    for dirpath, _, filenames in os.walk(pkg_root):
        for name in filenames:
            abs_path = os.path.join(dirpath, name)
            rel_path = os.path.relpath(abs_path, root)
            archive.write(abs_path, rel_path)
PY
        ;;
esac

rm -rf "$tmp_root"
