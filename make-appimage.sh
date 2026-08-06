#!/bin/bash
# Gera um AppImage portatil (Qt6 embutido) do CachyOS Unified Search.
# Requer: cmake, Qt6, curl. As ferramentas linuxdeploy sao baixadas em tools/.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build-appimage"
APPDIR="$BUILD_DIR/AppDir"
TOOLS_DIR="${LINUXDEPLOY_TOOLS_DIR:-tools}"
ICON_SOURCE="$SCRIPT_DIR/assets/icons/cachysearch.svg"

# linuxdeploy e ele proprio um AppImage; sem fuse2 no sistema, extrai e roda.
export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE=/usr/bin/qmake6
export OUTPUT="CachyOS_Unified_Search-x86_64.AppImage"

if [ ! -f "$ICON_SOURCE" ]; then
    echo "[appimage] Icone nao encontrado em: $ICON_SOURCE"
    exit 1
fi

# --- Baixa as ferramentas se necessario ---
mkdir -p "$TOOLS_DIR"
BASE="https://github.com/linuxdeploy"
if [ ! -f "$TOOLS_DIR/linuxdeploy.AppImage" ]; then
    echo "[appimage] Baixando linuxdeploy..."
    curl -sL -o "$TOOLS_DIR/linuxdeploy.AppImage" \
        "$BASE/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
fi
if [ ! -f "$TOOLS_DIR/linuxdeploy-plugin-qt.AppImage" ]; then
    echo "[appimage] Baixando linuxdeploy-plugin-qt..."
    curl -sL -o "$TOOLS_DIR/linuxdeploy-plugin-qt.AppImage" \
        "$BASE/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
fi
chmod +x "$TOOLS_DIR"/*.AppImage

# --- Compila em Release ---
echo "[appimage] Compilando..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc)"

# --- Monta o AppDir ---
echo "[appimage] Montando AppDir..."
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" \
         "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/scalable/apps"

cp "$BUILD_DIR/cachysearch_gui" "$APPDIR/usr/bin/"
cp "$ICON_SOURCE" "$APPDIR/usr/share/icons/hicolor/scalable/apps/"

cat > "$APPDIR/usr/share/applications/cachysearch.desktop" << 'DESKTOP'
[Desktop Entry]
Name=CachyOS Unified Search
Comment=Buscar e gerenciar pacotes Pacman, AUR e Flatpak
Exec=cachysearch_gui
Icon=cachysearch
Terminal=false
Type=Application
Categories=System;Utility;
StartupNotify=true
DESKTOP

# --- Empacota ---
echo "[appimage] Empacotando (Qt6 + plugins)..."
"$TOOLS_DIR/linuxdeploy.AppImage" \
    --appdir "$APPDIR" \
    --plugin qt \
    --output appimage

echo ""
echo "[appimage] Pronto: $SCRIPT_DIR/$OUTPUT"
echo "[appimage] Rode com: ./$OUTPUT"
echo "[appimage] (sem fuse2 instalado: ./$OUTPUT --appimage-extract-and-run)"
