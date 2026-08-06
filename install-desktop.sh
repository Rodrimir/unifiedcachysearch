#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BINARY="$SCRIPT_DIR/build/cachysearch_gui"
APP_ID="cachysearch"

echo "[cachysearch] Instalando atalho do CachyOS Unified Search..."

# --- Valida o binario ---
if [ ! -f "$BINARY" ]; then
    echo "[cachysearch] Binario nao encontrado em: $BINARY"
    echo "[cachysearch] Execute 'cmake --build build' primeiro, ou ajuste o caminho."
    exit 1
fi
chmod +x "$BINARY"

# --- Instala o icone SVG (fonte unica: assets/icons) ---
ICON_SOURCE="$SCRIPT_DIR/assets/icons/$APP_ID.svg"
ICON_DIR="$HOME/.local/share/icons/hicolor/scalable/apps"
ICON_PATH="$ICON_DIR/$APP_ID.svg"

if [ ! -f "$ICON_SOURCE" ]; then
    echo "[cachysearch] Icone nao encontrado em: $ICON_SOURCE"
    exit 1
fi

mkdir -p "$ICON_DIR"
cp "$ICON_SOURCE" "$ICON_PATH"

# --- Cria o arquivo .desktop ---
DESKTOP_DIR="$HOME/.local/share/applications"
DESKTOP_PATH="$DESKTOP_DIR/$APP_ID.desktop"
mkdir -p "$DESKTOP_DIR"

cat > "$DESKTOP_PATH" << DESKTOP
[Desktop Entry]
Name=CachyOS Unified Search
Comment=Buscar e gerenciar pacotes Pacman, AUR e Flatpak
Exec=$BINARY
Icon=$APP_ID
Terminal=false
Type=Application
Categories=System;Utility;
StartupNotify=true
DESKTOP
chmod +x "$DESKTOP_PATH"

# --- Atualiza caches do sistema ---
update-desktop-database -q "$DESKTOP_DIR" 2>/dev/null || true
gtk-update-icon-cache -f -t "$HOME/.local/share/icons" 2>/dev/null || true

# KDE Plasma
if command -v kbuildsycoca6 &>/dev/null; then
    kbuildsycoca6 --noincremental 2>/dev/null &
elif command -v kbuildsycoca5 &>/dev/null; then
    kbuildsycoca5 --noincremental 2>/dev/null &
fi

echo ""
echo "[cachysearch] OK! Atalho instalado:"
echo "  .desktop  -> $DESKTOP_PATH"
echo "  icone     -> $ICON_PATH"
echo ""
echo "[cachysearch] O app ja deve aparecer no menu como 'CachyOS Unified Search'."
echo "[cachysearch] Se nao aparecer, execute manualmente:"
echo "  kbuildsycoca6 --noincremental   (KDE)"
echo "  update-desktop-database ~/.local/share/applications   (outros DEs)"
