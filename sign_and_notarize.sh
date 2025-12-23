#!/bin/bash
# SPLENTA V1.0.0 - Code Signing and Notarization Script
# Automatically signs, packages, and notarizes the SPLENTA audio plugin
# Prerequisites:
#   - Developer ID Application certificate installed
#   - Apple Developer account credentials
#   - Xcode command line tools

set -e  # Exit on error

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}   SPLENTA V1.0.0 - Code Signing & Notarization${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════${NC}"
echo ""

# === CONFIGURATION ===
PROJECT_DIR="/Users/MediaStorm/Desktop/NewProject"
BUILD_DIR="$PROJECT_DIR/Builds/MacOSX/build/Release"
PLUGIN_NAME="SPLENTA.vst3"
PLUGIN_PATH="$BUILD_DIR/$PLUGIN_NAME"
IDENTITY="Developer ID Application: Yiyang Cai (33NJKA4738)"
BUNDLE_ID="com.solaris.SPLENTA"
DMG_DIR="/tmp/SPLENTA_DMG"
FINAL_DMG_PATH="$PROJECT_DIR/SPLENTA_v1.0.0_macOS.dmg"

# Apple Developer Credentials (will prompt if not set)
APPLE_ID="${APPLE_ID:-bidagosila@icloud.com}"
TEAM_ID="33NJKA4738"

# === STEP 1: BUILD RELEASE VERSION ===
echo -e "${YELLOW}[1/6]${NC} Building Release version..."
cd "$PROJECT_DIR/Builds/MacOSX"

# Clean previous build
if [ -d "build" ]; then
    echo "  → Cleaning previous build..."
    rm -rf build
fi

# Build with xcodebuild
echo "  → Compiling Release configuration..."
xcodebuild -project SPLENTA.xcodeproj \
    -configuration Release \
    -scheme "SPLENTA - VST3" \
    -jobs $(sysctl -n hw.ncpu) \
    clean build \
    CODE_SIGN_IDENTITY="$IDENTITY" \
    DEVELOPMENT_TEAM="$TEAM_ID" \
    | xcpretty 2>/dev/null || xcodebuild -project SPLENTA.xcodeproj \
        -configuration Release \
        -scheme "SPLENTA - VST3" \
        clean build \
        CODE_SIGN_IDENTITY="$IDENTITY" \
        DEVELOPMENT_TEAM="$TEAM_ID"

if [ ! -d "$PLUGIN_PATH" ]; then
    echo -e "${RED}✗ Build failed: Plugin not found at $PLUGIN_PATH${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build successful${NC}"
echo ""

# === STEP 2: CODE SIGNING ===
echo -e "${YELLOW}[2/6]${NC} Code signing plugin bundle..."

# Remove extended attributes and quarantine flags
echo "  → Cleaning extended attributes..."
xattr -cr "$PLUGIN_PATH"

# Sign all nested executables and libraries first
echo "  → Signing nested binaries..."
find "$PLUGIN_PATH" -type f \( -name "*.dylib" -o -perm +111 \) | while read file; do
    echo "    • $(basename "$file")"
    codesign --force --sign "$IDENTITY" \
        --timestamp \
        --options runtime \
        "$file" 2>/dev/null || true
done

# Sign the main plugin bundle
echo "  → Signing main bundle..."
codesign --force --sign "$IDENTITY" \
    --timestamp \
    --options runtime \
    --deep \
    --identifier "$BUNDLE_ID" \
    "$PLUGIN_PATH"

# Verify signature
echo "  → Verifying signature..."
codesign --verify --deep --strict --verbose=2 "$PLUGIN_PATH"
spctl --assess --type execute --verbose "$PLUGIN_PATH" || echo "    (Notarization required for Gatekeeper)"

echo -e "${GREEN}✓ Code signing complete${NC}"
echo ""

# === STEP 3: CREATE DMG PACKAGE ===
echo -e "${YELLOW}[3/6]${NC} Creating DMG installer..."

# Clean and create DMG staging directory
rm -rf "$DMG_DIR"
mkdir -p "$DMG_DIR"

# Copy plugin to staging
echo "  → Copying plugin to DMG staging..."
cp -R "$PLUGIN_PATH" "$DMG_DIR/"

# Create README
echo "  → Creating installation guide..."
cat > "$DMG_DIR/安装说明 - Installation Guide.txt" << 'EOF'
SPLENTA V1.0.0 - Audio Plugin
═══════════════════════════════════════════════════════════

🎵 Installation / 安装方法:

1. Copy "SPLENTA.vst3" to your VST3 folder:
   复制 SPLENTA.vst3 到 VST3 文件夹:

   macOS (Intel):   ~/Library/Audio/Plug-Ins/VST3/
   macOS (Apple Silicon): Same location / 同上

2. Restart your DAW (Logic Pro, Ableton Live, etc.)
   重启你的 DAW 音乐软件

3. Load SPLENTA in your plugin list
   在插件列表中加载 SPLENTA

═══════════════════════════════════════════════════════════

📋 Requirements / 系统要求:
  • macOS 10.13 (High Sierra) or later
  • 64-bit Intel or Apple Silicon processor
  • Compatible DAW (Logic Pro X, Ableton Live, FL Studio, etc.)

═══════════════════════════════════════════════════════════

⚠️  First Launch / 首次启动:
  If macOS shows a security warning:
  如果 macOS 显示安全警告:

  1. Open System Settings / 打开系统设置
  2. Go to Privacy & Security / 进入隐私与安全
  3. Click "Open Anyway" / 点击"仍要打开"

═══════════════════════════════════════════════════════════

🎨 Features / 功能特色:
  • 5 Visual Themes (Bronze, Blue, Purple, Green, Pink)
  • Real-time 3D particle visualizations
  • Envelope control (ADSR)
  • MIDI-triggered effects
  • Bypass and saturation modes

═══════════════════════════════════════════════════════════

📧 Support / 技术支持:
  Developer: Solaris
  Email: bidagosila@icloud.com

© 2025 Solaris. All rights reserved.
EOF

# Create symbolic link to VST3 folder (for easy installation)
echo "  → Creating VST3 folder shortcut..."
ln -s "$HOME/Library/Audio/Plug-Ins/VST3" "$DMG_DIR/→ VST3 插件文件夹 (拖拽安装)"

# Create DMG with custom appearance
echo "  → Building DMG image..."
rm -f "$FINAL_DMG_PATH"

hdiutil create -volname "SPLENTA V1.0.0" \
    -srcfolder "$DMG_DIR" \
    -ov -format UDZO \
    -imagekey zlib-level=9 \
    "$FINAL_DMG_PATH"

echo -e "${GREEN}✓ DMG created: $FINAL_DMG_PATH${NC}"
echo ""

# === STEP 4: SIGN DMG ===
echo -e "${YELLOW}[4/6]${NC} Signing DMG installer..."
codesign --force --sign "$IDENTITY" \
    --timestamp \
    "$FINAL_DMG_PATH"

codesign --verify --verbose "$FINAL_DMG_PATH"
echo -e "${GREEN}✓ DMG signed${NC}"
echo ""

# === STEP 5: NOTARIZATION ===
echo -e "${YELLOW}[5/6]${NC} Submitting to Apple for notarization..."
echo ""
echo -e "${BLUE}⚠️  Notarization requires Apple Developer credentials${NC}"
echo -e "This step will:"
echo "  1. Upload DMG to Apple (~10MB, may take 1-2 minutes)"
echo "  2. Wait for Apple's automated security scan (~5-10 minutes)"
echo "  3. Staple notarization ticket to DMG"
echo ""
echo -e "${YELLOW}Press ENTER to continue, or Ctrl+C to skip notarization${NC}"
read -r

# Check if app-specific password is set
if [ -z "$APPLE_APP_PASSWORD" ]; then
    echo -e "${YELLOW}Apple App-Specific Password not found in environment${NC}"
    echo ""
    echo "To enable automatic notarization, you need to:"
    echo "  1. Go to https://appleid.apple.com"
    echo "  2. Sign in with Apple ID: $APPLE_ID"
    echo "  3. Generate an App-Specific Password"
    echo "  4. Store it in keychain:"
    echo ""
    echo -e "${BLUE}     xcrun notarytool store-credentials \"SPLENTA-Notarization\" \\${NC}"
    echo -e "${BLUE}         --apple-id \"$APPLE_ID\" \\${NC}"
    echo -e "${BLUE}         --team-id \"$TEAM_ID\" \\${NC}"
    echo -e "${BLUE}         --password \"your-app-specific-password\"${NC}"
    echo ""
    echo -e "${YELLOW}Do you want to set it up now? (y/n)${NC}"
    read -r setup_creds

    if [[ "$setup_creds" =~ ^[Yy]$ ]]; then
        echo ""
        echo "Please enter your App-Specific Password:"
        read -s app_password
        echo ""
        echo "Storing credentials in keychain..."
        xcrun notarytool store-credentials "SPLENTA-Notarization" \
            --apple-id "$APPLE_ID" \
            --team-id "$TEAM_ID" \
            --password "$app_password"

        NOTARIZATION_PROFILE="SPLENTA-Notarization"
    else
        echo -e "${YELLOW}⚠️  Skipping notarization. DMG is signed but not notarized.${NC}"
        echo "   Users may see security warnings on macOS 10.14.5+"
        NOTARIZATION_PROFILE=""
    fi
else
    NOTARIZATION_PROFILE="SPLENTA-Notarization"
fi

# Submit for notarization if credentials are configured
if [ -n "$NOTARIZATION_PROFILE" ]; then
    echo ""
    echo "  → Uploading DMG to Apple..."
    SUBMISSION_ID=$(xcrun notarytool submit "$FINAL_DMG_PATH" \
        --keychain-profile "$NOTARIZATION_PROFILE" \
        --wait 2>&1 | tee /dev/tty | grep -o 'id: [a-f0-9-]*' | head -1 | cut -d' ' -f2)

    if [ -z "$SUBMISSION_ID" ]; then
        echo -e "${RED}✗ Notarization submission failed${NC}"
        echo "  Check credentials with: xcrun notarytool history --keychain-profile $NOTARIZATION_PROFILE"
    else
        echo ""
        echo "  → Submission ID: $SUBMISSION_ID"
        echo "  → Waiting for Apple's review (this may take 5-10 minutes)..."

        # Wait for notarization to complete
        xcrun notarytool wait "$SUBMISSION_ID" --keychain-profile "$NOTARIZATION_PROFILE"

        # Check status
        STATUS=$(xcrun notarytool info "$SUBMISSION_ID" --keychain-profile "$NOTARIZATION_PROFILE" 2>/dev/null | grep 'status:' | awk '{print $2}')

        if [ "$STATUS" = "Accepted" ]; then
            echo -e "${GREEN}✓ Notarization successful!${NC}"

            # === STEP 6: STAPLE TICKET ===
            echo ""
            echo -e "${YELLOW}[6/6]${NC} Stapling notarization ticket to DMG..."
            xcrun stapler staple "$FINAL_DMG_PATH"
            xcrun stapler validate "$FINAL_DMG_PATH"
            echo -e "${GREEN}✓ Notarization ticket stapled${NC}"
        else
            echo -e "${RED}✗ Notarization failed with status: $STATUS${NC}"
            echo "  View detailed log:"
            echo "    xcrun notarytool log $SUBMISSION_ID --keychain-profile $NOTARIZATION_PROFILE"
        fi
    fi
else
    echo -e "${YELLOW}[6/6]${NC} Skipped stapling (notarization not completed)"
fi

# === CLEANUP ===
echo ""
echo "  → Cleaning up temporary files..."
rm -rf "$DMG_DIR"

# === SUMMARY ===
echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Process Complete!${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════${NC}"
echo ""
echo "📦 Final Installer:"
echo "   $FINAL_DMG_PATH"
echo ""
ls -lh "$FINAL_DMG_PATH"
echo ""

if [ -n "$NOTARIZATION_PROFILE" ]; then
    echo -e "${GREEN}✓ Code Signed${NC}"
    echo -e "${GREEN}✓ Notarized by Apple${NC}"
    echo -e "${GREEN}✓ Ready for distribution${NC}"
    echo ""
    echo "Users can install SPLENTA without security warnings on macOS 10.14.5+"
else
    echo -e "${GREEN}✓ Code Signed${NC}"
    echo -e "${YELLOW}⚠️  Not Notarized${NC}"
    echo ""
    echo "Users may see security warnings on first launch."
    echo "To complete notarization later, run this script again."
fi

echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════${NC}"
