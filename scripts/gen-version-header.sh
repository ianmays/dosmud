#!/bin/sh
set -eu

if [ "$#" -ne 2 ]; then
    echo "usage: $0 <VERSION file> <output header>" >&2
    exit 1
fi

version_file=$1
out_file=$2

base_version=$(tr -d '\r\n' < "$version_file")
if [ -z "$base_version" ]; then
    echo "empty VERSION file" >&2
    exit 1
fi

build_version="${base_version}+local"

if command -v git >/dev/null 2>&1 &&
        git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    count=$(git rev-list --count HEAD 2>/dev/null || true)
    sha=$(git rev-parse --short=7 HEAD 2>/dev/null || true)
    dirty=""

    if [ -n "$count" ] && [ -n "$sha" ]; then
        if ! git diff --quiet --ignore-submodules HEAD -- 2>/dev/null ||
                ! git diff --quiet --ignore-submodules --cached -- 2>/dev/null; then
            dirty=".dirty"
        fi
        build_version="${base_version}+build.${count}.g${sha}${dirty}"
    fi
fi

mkdir -p "$(dirname "$out_file")"
cat > "$out_file" <<EOF
#ifndef VERSION_H
#define VERSION_H

#define BUILD_BASE_VERSION "${base_version}"
#define BUILD_VERSION_STRING "${build_version}"

#endif
EOF
