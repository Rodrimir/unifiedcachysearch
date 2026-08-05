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

# --- Gera o icone SVG ---
ICON_DIR="$HOME/.local/share/icons/hicolor/scalable/apps"
ICON_PATH="$ICON_DIR/$APP_ID.svg"
mkdir -p "$ICON_DIR"

cat > "$ICON_PATH" << 'SVG'
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128">
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0%" stop-color="#313244"/>
      <stop offset="100%" stop-color="#1e1e2e"/>
    </linearGradient>
  </defs>
  <rect width="128" height="128" rx="24" fill="url(#bg)"/>
  <circle cx="52" cy="52" r="28" fill="none" stroke="#89b4fa" stroke-width="6"/>
  <circle cx="52" cy="52" r="12" fill="#89b4fa" opacity="0.3"/>
  <line x1="72" y1="72" x2="96" y2="96" stroke="#89b4fa" stroke-width="7" stroke-linecap="round"/>
  <text x="64" y="118" font-family="sans-serif" font-size="12" fill="#a6adc8" text-anchor="middle">CACHY</text>
</svg>
SVG

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
