#!/bin/sh

# From Azure-pipelines.yml
OBS_DEPS_VERSION=2020-12-22
QT_VERSION=5.15.2


OSTYPE=$(uname)

if [ "${OSTYPE}" != "Darwin" ]; then
	echo "[loupedeck-obs - Error] macOS install dependencies script can be run on Darwin-type OS only."
	exit 1
fi

HAS_BREW=$(type brew 2>/dev/null)

if [ "${HAS_BREW}" = "" ]; then
	echo "[loupedeck-obs - Error] Please install Homebrew (https://www.brew.sh/) to build loupedeck-obs on macOS."
	exit 1
fi

# OBS Studio Brew Deps
echo "[loupedeck-obs] Updating Homebrew.."
brew update >/dev/null
echo "[loupedeck-obs] Checking installed Homebrew formulas.."

if [ -d /usr/local/opt/openssl@1.0.2t ]; then
  brew uninstall openssl@1.0.2t
  brew untap local/openssl
fi

if [ -d /usr/local/opt/python@2.7.17 ]; then
  brew uninstall python@2.7.17
  brew untap local/python2
fi

brew bundle --file ./CI/macos/Brewfile

# Fetch and install Packages app
# =!= NOTICE =!=
# Installs a LaunchDaemon under /Library/LaunchDaemons/fr.whitebox.packages.build.dispatcher.plist
# =!= NOTICE =!=

HAS_PACKAGES=$(type packagesbuild 2>/dev/null)

if [ "${HAS_PACKAGES}" = "" ]; then
	echo "[loupedeck-obs] Installing Packaging app (might require password due to 'sudo').."
	curl -L -O http://s.sudre.free.fr/Software/files/Packages.dmg
	sudo hdiutil attach ./Packages.dmg
	sudo installer -pkg /Volumes/Packages\ 1.2.9/Install\ Packages.pkg -target /
fi

# OBS Deps
echo "[loupedeck-obs] Installing loupedeck-obs dependency 'OBS Deps ${OBS_DEPS_VERSION}'.."
wget --quiet --retry-connrefused --waitretry=1 https://github.com/obsproject/obs-deps/releases/download/${OBS_DEPS_VERSION}/macos-deps-${OBS_DEPS_VERSION}.tar.gz
tar -xf ./macos-deps-${OBS_DEPS_VERSION}.tar.gz -C /tmp

# Qt deps
echo "[loupedeck-obs] Installing loupedeck-obs dependency 'Qt ${QT_VERSION}'.."
curl -L -O https://github.com/obsproject/obs-deps/releases/download/${OBS_DEPS_VERSION}/macos-qt-${QT_VERSION}-${OBS_DEPS_VERSION}.tar.gz
tar -xf ./macos-qt-${QT_VERSION}-${OBS_DEPS_VERSION}.tar.gz -C "/tmp"
xattr -r -d com.apple.quarantine /tmp/obsdeps