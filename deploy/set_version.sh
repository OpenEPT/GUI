#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 2.0.3"
    exit 1
fi

VERSION="$1"

if ! [[ "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "ERROR: version must look like 2.0.3" >&2
    exit 1
fi

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_DIR}"

if [ -n "$(git status --porcelain)" ]; then
    echo "ERROR: working tree is not clean. Commit or stash your changes first." >&2
    exit 1
fi

if git rev-parse "v${VERSION}" >/dev/null 2>&1; then
    echo "ERROR: tag v${VERSION} already exists." >&2
    exit 1
fi

echo "${VERSION}" > VERSION

git add VERSION
git commit -m "Release ${VERSION}"
git tag -a "v${VERSION}" -m "OpenEPT ${VERSION}"

echo
echo "VERSION set to ${VERSION}, commit and tag v${VERSION} created."
echo
echo "Push with:"
echo
echo "  git push && git push origin v${VERSION}"
