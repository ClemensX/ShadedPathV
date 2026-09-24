#!/usr/bin/env bash
set -euo pipefail

### Build Mac bundle, should be shippable alone and run w/o needing to install anything else

# --- Configuration: adjust these paths for your machine ---
BUILD_DIR="out/build/mac-release/src/app"
EXECUTABLE_NAME="app"                      # target name from src/app/CMakeLists.txt
APP_NAME="YourAwesomeGame"
VULKAN_SDK="${VULKAN_SDK:-$HOME/VulkanSDK/1.4.0.0}"
KOSMICKRISP_DYLIB="/path/to/libvulkan_kosmickrisp.dylib"
KOSMICKRISP_ICD_JSON="/path/to/libkosmickrisp_icd.json"
ASSETS_DIR="assets"                        # adjust to match Files::getAssetFolderPath()
SHADER_BIN_DIR="${BUILD_DIR}/shader.bin"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

APP="${APP_NAME}.app"
CONTENTS="${APP}/Contents"
MACOS_DIR="${CONTENTS}/MacOS"
FRAMEWORKS_DIR="${CONTENTS}/Frameworks"
RESOURCES_DIR="${CONTENTS}/Resources"
ICD_DIR="${RESOURCES_DIR}/vulkan/icd.d"

echo "==> Cleaning previous bundle"
rm -rf "${APP}"

echo "==> Creating bundle skeleton"
mkdir -p "${MACOS_DIR}" "${FRAMEWORKS_DIR}" "${ICD_DIR}"

echo "==> Copying executable"
cp "${BUILD_DIR}/${EXECUTABLE_NAME}" "${MACOS_DIR}/${APP_NAME}"
chmod +x "${MACOS_DIR}/${APP_NAME}"

echo "==> Copying Vulkan runtime libraries"
cp "${VULKAN_SDK}/macOS/lib/libMoltenVK.dylib" "${FRAMEWORKS_DIR}/"
cp -L "${VULKAN_SDK}/macOS/lib/libvulkan.1.dylib" "${FRAMEWORKS_DIR}/libvulkan.1.dylib"
cp "${KOSMICKRISP_DYLIB}" "${FRAMEWORKS_DIR}/"

echo "==> Copying ICD manifests"
cp "${VULKAN_SDK}/macOS/share/vulkan/icd.d/MoltenVK_icd.json" "${ICD_DIR}/"
cp "${KOSMICKRISP_ICD_JSON}" "${ICD_DIR}/"

# Rewrite library_path in each manifest to a bundle-relative path.
# icd.d -> vulkan -> Resources -> Contents, so 3 levels up to Contents, then into Frameworks.
sed -i '' 's#"library_path": *".*"#"library_path": "../../../Frameworks/libMoltenVK.dylib"#' "${ICD_DIR}/MoltenVK_icd.json"
sed -i '' 's#"library_path": *".*"#"library_path": "../../../Frameworks/libvulkan_kosmickrisp.dylib"#' "${ICD_DIR}/libkosmickrisp_icd.json"

echo "==> Copying shaders and assets"
mkdir -p "${RESOURCES_DIR}/shader.bin"
cp -R "${SHADER_BIN_DIR}/." "${RESOURCES_DIR}/shader.bin/"
if [ -d "${ASSETS_DIR}" ]; then
  cp -R "${ASSETS_DIR}" "${RESOURCES_DIR}/assets"
fi

echo "==> Writing Info.plist"
cat > "${CONTENTS}/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleExecutable</key><string>${APP_NAME}</string>
  <key>CFBundleIdentifier</key><string>com.yourname.yourawesomegame</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>1.0</string>
  <key>CFBundleVersion</key><string>1</string>
  <key>LSMinimumSystemVersion</key><string>13.0</string>
  <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
PLIST

echo "==> Fixing dylib IDs and rpaths"
for lib in "${FRAMEWORKS_DIR}"/*.dylib; do
  install_name_tool -id "@rpath/$(basename "${lib}")" "${lib}"
done

install_name_tool -add_rpath "@executable_path/../Frameworks" "${MACOS_DIR}/${APP_NAME}"

echo "==> Rewriting absolute Vulkan SDK references to @rpath"
# Adjust/extend this loop based on what 'otool -L' actually reports for your build.
OLD_VULKAN_LOADER_PATH="${VULKAN_SDK}/macOS/lib/libvulkan.1.dylib"
if otool -L "${MACOS_DIR}/${APP_NAME}" | grep -q "${OLD_VULKAN_LOADER_PATH}"; then
  install_name_tool -change "${OLD_VULKAN_LOADER_PATH}" "@rpath/libvulkan.1.dylib" "${MACOS_DIR}/${APP_NAME}"
fi

echo "==> otool -L report (verify no absolute SDK paths remain):"
otool -L "${MACOS_DIR}/${APP_NAME}"
for lib in "${FRAMEWORKS_DIR}"/*.dylib; do
  echo "--- ${lib} ---"
  otool -L "${lib}"
done

echo "==> Ad-hoc code signing (replace with Developer ID cert for real distribution)"
codesign --deep --force --sign - "${APP}"

echo "==> Packaging as zip"
ditto -c -k --sequesterRsrc --keepParent "${APP}" "${APP_NAME}.zip"

echo "==> Done: ${APP} and ${APP_NAME}.zip"