#!/bin/bash

set -e

OSTYPE=$(uname)

#AL
RELEASE_MODE=false


if [ "${OSTYPE}" != "Darwin" ]; then
	echo "[loupedeck-obs - Error] macOS package script can be run on Darwin-type OS only."
	exit 1
fi

echo "[loupedeck-obs] Preparing package build"

GIT_HASH=$(git rev-parse --short HEAD)
GIT_BRANCH_OR_TAG=$(git name-rev --name-only HEAD | awk -F/ '{print $NF}')

VERSION="$GIT_HASH-$GIT_BRANCH_OR_TAG"

FILENAME_UNSIGNED="loupedeck-obs-$VERSION-Unsigned.pkg"
FILENAME="loupedeck-obs-$VERSION.pkg"

echo "[loupedeck-obs] Modifying loupedeck-obs.so linking"
install_name_tool \
	-change /tmp/obsdeps/lib/QtWidgets.framework/Versions/5/QtWidgets \
		@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets \
	-change /tmp/obsdeps/lib/QtGui.framework/Versions/5/QtGui \
		@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui \
	-change /tmp/obsdeps/lib/QtCore.framework/Versions/5/QtCore \
		@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore \
	./build/loupedeck-obs.so

# Check if replacement worked
echo "[loupedeck-obs] Dependencies for loupedeck-obs"
otool -L ./build/loupedeck-obs.so

if [[ "$RELEASE_MODE" == "True" ]]; then
	echo "[loupedeck-obs] Signing plugin binary: loupedeck-obs.so"
	codesign --sign "$CODE_SIGNING_IDENTITY" ./build/loupedeck-obs.so
else
	echo "[loupedeck-obs] Skipped plugin codesigning"
fi

echo "[loupedeck-obs] Actual package build"
packagesbuild ./CI/macos/loupedeck-obs.pkgproj

echo "[loupedeck-obs] Renaming loupedeck-obs.pkg to $FILENAME"
mv ./release/loupedeck-obs.pkg ./release/$FILENAME_UNSIGNED

if [[ "$RELEASE_MODE" == "True" ]]; then
	echo "[loupedeck-obs] Signing installer: $FILENAME"
	productsign \
		--sign "$INSTALLER_SIGNING_IDENTITY" \
		./release/$FILENAME_UNSIGNED \
		./release/$FILENAME
	rm ./release/$FILENAME_UNSIGNED

	echo "[loupedeck-obs] Submitting installer $FILENAME for notarization"
	zip -r ./release/$FILENAME.zip ./release/$FILENAME
	UPLOAD_RESULT=$(xcrun altool \
		--notarize-app \
		--primary-bundle-id "com.loupedeck.loupedeck-obs" \


		--username "$AC_USERNAME" \
		--password "$AC_PASSWORD" \
		--asc-provider "$AC_PROVIDER_SHORTNAME" \
		--file "./release/$FILENAME.zip")
	rm ./release/$FILENAME.zip

	REQUEST_UUID=$(echo $UPLOAD_RESULT | awk -F ' = ' '/RequestUUID/ {print $2}')
	echo "Request UUID: $REQUEST_UUID"

	echo "[loupedeck-obs] Wait for notarization result"
	# Pieces of code borrowed from rednoah/notarized-app
	while sleep 30 && date; do
		CHECK_RESULT=$(xcrun altool \
			--notarization-info "$REQUEST_UUID" \
			--username "$AC_USERNAME" \
			--password "$AC_PASSWORD" \
			--asc-provider "$AC_PROVIDER_SHORTNAME")
		echo $CHECK_RESULT

		if ! grep -q "Status: in progress" <<< "$CHECK_RESULT"; then
			echo "[loupedeck-obs] Staple ticket to installer: $FILENAME"
			xcrun stapler staple ./release/$FILENAME
			break
		fi
	done
else
	echo "[loupedeck-obs] Skipped installer codesigning and notarization"
fi