#!/bin/bash

# 1. CHECK INPUT
if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 0.4.1"
    exit 1
fi

VERSION=$1
APP_NAME="cangaroo"
BINARY_PATH="./bin/${APP_NAME}" 
ARCH=$(uname -m)
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
RELEASE_NAME="${APP_NAME}-v${VERSION}-${OS}-${ARCH}"
TAR_FILE="${RELEASE_NAME}.tar.gz"

# 2. CHECK IF BINARY EXISTS
if [ ! -f "$BINARY_PATH" ]; then
    echo "Error: Binary not found at $BINARY_PATH"
    echo "Please build the project first."
    exit 1
fi

echo "Packaging ${APP_NAME} version ${VERSION}..."

# 3. CREATE STAGING DIRECTORY
mkdir -p release_stage
cp "$BINARY_PATH" release_stage/
cp README.md release_stage/
cp LICENSE release_stage/
cp CONTRIBUTING.md release_stage/

# 4. CREATE TARBALL
# -C changes directory so the tar doesn't include the 'release_stage' parent folder path
tar -czf "$TAR_FILE" -C release_stage .

# 5. GENERATE CHECKSUM
sha256sum "$TAR_FILE" > "${TAR_FILE}.sha256"

# 6. CLEANUP & OUTPUT
rm -rf release_stage

echo "-------------------------------------------------------"
echo "Release Created Successfully:"
echo "Package:  $TAR_FILE"
echo "Checksum: ${TAR_FILE}.sha256"
echo "-------------------------------------------------------"
echo "SHA256 Output:"
cat "${TAR_FILE}.sha256"
echo "-------------------------------------------------------"
