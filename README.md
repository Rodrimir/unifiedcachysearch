# CachyOS Unified Search

Busca unificada de pacotes **Pacman**, **AUR** e **Flatpak** em uma única
interface Qt6, para Arch Linux / CachyOS.

- Busca nas três fontes em paralelo, com ranking por relevância
- Cross-reference: mostra que um pacote existe no repositório oficial *e* no AUR
- Aba de atualizações (`checkupdates`, `paru -Qua`, `flatpak remote-ls --updates`)
- Instalação/remoção com console embutido, tema claro/escuro e exportação
  JSON/CSV/TXT

## Requisitos

Qt6 (Widgets, Concurrent, Network, Multimedia), CMake ≥ 3.20, `pacman`.
Opcionais: `paru` ou `yay` (AUR) e `flatpak`.

## Uso

```bash
./run.sh                 # compila em build/ e executa
./make-appimage.sh       # gera o AppImage portátil (Qt6 embutido)
./install-desktop.sh     # instala o atalho no menu de aplicações
```

Testes:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

A organização do código está descrita em [ESTRUTURA.md](ESTRUTURA.md).
