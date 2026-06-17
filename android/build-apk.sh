#!/bin/bash
# SkyNet Android APK Builder
# Builds the SSI Runtime Android app using Gradle.
#
# Prerequisites:
#   - Java 17+ (available at /c/Users/123/WorkBuddy/.../jdk-17.0.9+9/bin/java)
#   - Android SDK (set ANDROID_HOME)
#
# Usage:
#   ./gradlew assembleRelease    # Build release APK
#   ./gradlew assembleDebug      # Build debug APK
#
# Output: app/build/outputs/apk/

echo "========================================="
echo " SkyNet SSI Runtime — Android APK Build"
echo "========================================="
echo ""

# Check Java
JAVA_BIN=$(which java 2>/dev/null)
if [[ -z "${JAVA_BIN}" ]]; then
    echo "[ERROR] Java not found. Set JAVA_HOME or add java to PATH."
    exit 1
fi
echo "[OK] Java: $(${JAVA_BIN} -version 2>&1 | head -1)"

# Check Android SDK
if [[ -z "${ANDROID_HOME}" ]]; then
    if [[ -d "/d/Android/Sdk" ]]; then
        export ANDROID_HOME="/d/Android/Sdk"
    elif [[ -d "${HOME}/Android/Sdk" ]]; then
        export ANDROID_HOME="${HOME}/Android/Sdk"
    else
        echo "[WARN] ANDROID_HOME not set. If SDK is installed, set:"
        echo "  export ANDROID_HOME=/path/to/android/sdk"
        echo ""
        echo "You can install the SDK via Android Studio or sdkmanager."
        echo "Minimum required: build-tools 34.0.0, platforms android-34"
    fi
fi

if [[ -n "${ANDROID_HOME}" ]]; then
    echo "[OK] Android SDK: ${ANDROID_HOME}"
fi

echo ""
echo "Building..."

# Make gradlew executable if it exists
if [[ -f "gradlew" ]]; then
    chmod +x gradlew
    ./gradlew assembleDebug
else
    echo "[INFO] No gradlew found. Generate it or use:"
    echo "  gradle wrapper"
    echo ""
    echo "Then run: ./gradlew assembleDebug"
    echo ""
    echo "APK output: app/build/outputs/apk/debug/"
fi
