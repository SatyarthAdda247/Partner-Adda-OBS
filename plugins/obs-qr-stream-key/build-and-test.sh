#!/bin/bash

set -e

echo "=========================================="
echo "OBS QR Stream Key Plugin - Build & Test"
echo "=========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Step 1: Check if httplib.h exists
echo "Step 1: Checking for httplib.h..."
if [ ! -f "httplib.h" ]; then
    echo -e "${YELLOW}httplib.h not found. Downloading...${NC}"
    ./download-httplib.sh
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ httplib.h downloaded successfully${NC}"
    else
        echo -e "${RED}✗ Failed to download httplib.h${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}✓ httplib.h found${NC}"
fi
echo ""

# Step 2: Check dependencies
echo "Step 2: Checking dependencies..."

# Check for Qt6
if command -v qmake6 &> /dev/null || command -v qmake &> /dev/null; then
    echo -e "${GREEN}✓ Qt6 found${NC}"
else
    echo -e "${YELLOW}⚠ Qt6 not found in PATH${NC}"
fi

# Check for OpenSSL
if command -v openssl &> /dev/null; then
    echo -e "${GREEN}✓ OpenSSL found${NC}"
else
    echo -e "${YELLOW}⚠ OpenSSL not found in PATH${NC}"
fi

# Check for CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1)
    echo -e "${GREEN}✓ CMake found: $CMAKE_VERSION${NC}"
else
    echo -e "${RED}✗ CMake not found${NC}"
    exit 1
fi
echo ""

# Step 3: Determine platform and preset
echo "Step 3: Detecting platform..."
PLATFORM=$(uname -s)
case "$PLATFORM" in
    Darwin*)
        PRESET="macos"
        BUILD_DIR="build_macos"
        echo -e "${GREEN}✓ Platform: macOS${NC}"
        ;;
    Linux*)
        PRESET="ubuntu"
        BUILD_DIR="build_ubuntu"
        echo -e "${GREEN}✓ Platform: Linux${NC}"
        ;;
    *)
        echo -e "${RED}✗ Unsupported platform: $PLATFORM${NC}"
        exit 1
        ;;
esac
echo ""

# Step 4: Navigate to OBS root
echo "Step 4: Navigating to OBS root directory..."
cd ../../..
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}✗ Not in OBS Studio directory${NC}"
    exit 1
fi
echo -e "${GREEN}✓ In OBS Studio root${NC}"
echo ""

# Step 5: Configure CMake
echo "Step 5: Configuring CMake..."
if cmake --preset $PRESET; then
    echo -e "${GREEN}✓ CMake configuration successful${NC}"
else
    echo -e "${RED}✗ CMake configuration failed${NC}"
    exit 1
fi
echo ""

# Step 6: Build plugin
echo "Step 6: Building obs-qr-stream-key plugin..."
if cmake --build $BUILD_DIR --target obs-qr-stream-key --config RelWithDebInfo; then
    echo -e "${GREEN}✓ Plugin built successfully${NC}"
else
    echo -e "${RED}✗ Plugin build failed${NC}"
    exit 1
fi
echo ""

# Step 7: Check plugin file
echo "Step 7: Verifying plugin file..."
PLUGIN_PATH="$BUILD_DIR/plugins/obs-qr-stream-key/RelWithDebInfo/obs-qr-stream-key.so"
if [ -f "$PLUGIN_PATH" ]; then
    PLUGIN_SIZE=$(du -h "$PLUGIN_PATH" | cut -f1)
    echo -e "${GREEN}✓ Plugin file exists: $PLUGIN_PATH ($PLUGIN_SIZE)${NC}"
else
    echo -e "${RED}✗ Plugin file not found at: $PLUGIN_PATH${NC}"
    exit 1
fi
echo ""

# Step 8: Test backend connectivity
echo "Step 8: Testing backend connectivity..."

echo -n "  Testing Adda247 backend... "
if curl -s -o /dev/null -w "%{http_code}" -X POST https://admin.adda247.com/api/users/qr-login/generate | grep -q "200\|400\|401"; then
    echo -e "${GREEN}✓ Reachable${NC}"
else
    echo -e "${YELLOW}⚠ Not reachable (may be blocked)${NC}"
fi

echo -n "  Testing Vercel backend... "
if curl -s -o /dev/null -w "%{http_code}" -H "x-auth-token: fpoa43edty5" https://obs-backend-chi.vercel.app/api/session?token=test | grep -q "200\|400\|401"; then
    echo -e "${GREEN}✓ Reachable${NC}"
else
    echo -e "${YELLOW}⚠ Not reachable (may be blocked)${NC}"
fi
echo ""

# Step 9: Summary
echo "=========================================="
echo "Build Summary"
echo "=========================================="
echo -e "Platform:     ${GREEN}$PLATFORM${NC}"
echo -e "Preset:       ${GREEN}$PRESET${NC}"
echo -e "Build Dir:    ${GREEN}$BUILD_DIR${NC}"
echo -e "Plugin:       ${GREEN}$PLUGIN_PATH${NC}"
echo ""
echo -e "${GREEN}✓ Build completed successfully!${NC}"
echo ""
echo "Next steps:"
echo "  1. Run OBS: $BUILD_DIR/rundir/RelWithDebInfo/bin/64bit/obs"
echo "  2. Look for the authentication dialog on startup"
echo "  3. Or access via Tools → QR Authentication"
echo ""
echo "Logs location:"
echo "  Check OBS logs for plugin messages: [obs-qr-stream-key]"
echo ""
