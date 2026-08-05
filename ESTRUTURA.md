# Estrutura do Projeto — CachyOS Unified Search

```
cachySearch/
|
|-- cachysearch-gui/                 # Diretorio principal do codigo
|   |
|   |-- CMakeLists.txt               # Configuracao de build (Qt6, C++17)
|   |-- sounds.qrc                   # Recursos Qt: sons WAV + estilos QSS
|   |-- run.sh                       # Script para compilar e executar
|   |-- pack.sh                      # Empacotamento tar.gz
|   |-- install-desktop.sh           # Instala atalho no menu de aplicacoes
|   |-- ESTRUTURA.md                 # Documentacao da estrutura
|   |
|   |-- sounds/                      # Efeitos sonoros
|   |   |-- success.wav              #   Som de operacao bem-sucedida
|   |   |-- error.wav                #   Som de erro
|   |   |-- click.wav                #   Som de clique
|   |
|   |-- src/                         # Codigo-fonte C++
|   |   |
|   |   |-- core/                    # Dados, modelos, estruturas
|   |   |   |-- searchitem.h/.cpp          # Estrutura de dados de cada pacote
|   |   |   |-- searchmodel.h/.cpp         # Modelo Qt (QAbstractListModel)
|   |   |   |-- sortfilterproxymodel.h/.cpp# Filtragem e ordenacao
|   |   |   |-- theme.h                    # Paleta de cores Catppuccin
|   |   |
|   |   |-- engine/                  # Logica de busca e execucao
|   |   |   |-- searchengine.h/.cpp        # Motor: pacman, paru (AUR), flatpak
|   |   |   |-- searchcoordinator.h/.cpp   # Orquestra pesquisas e comandos
|   |   |   |-- commandrunner.h/.cpp       # Execucao assincrona de processos
|   |   |
|   |   |-- ui/                      # Widgets da interface
|   |   |   |-- mainwindow.h/.cpp          # Janela principal + entry point main()
|   |   |   |-- detailpanel.h/.cpp         # Painel lateral com detalhes
|   |   |   |-- outputconsole.h/.cpp       # Console com realce de erros
|   |   |   |-- packagedelegate.h/.cpp     # Renderizacao customizada
|   |   |
|   |   |-- audio/                   # Audio
|   |   |   |-- soundmanager.h/.cpp        # Singleton de efeitos sonoros
|   |   |
|   |   |-- styles/                  # Folhas de estilo Qt QSS
|   |       |-- dark.qss             #   Tema escuro (Catppuccin Mocha)
|   |       |-- light.qss            #   Tema claro (Catppuccin Latte)
|   |
|   |-- tests/                       # Testes automatizados
|   |   |-- CMakeLists.txt
|   |   |-- test_search.cpp          # Testes do motor de busca
|   |
|   |-- build/                       # Artefatos de compilacao (gerado)
|       |-- cachysearch_gui          #   Binario principal compilado
|       |-- tests/test_search        #   Binario de teste
|
|-- ESTRUTURA.md (link simbolico)    # Este arquivo
```

## Resumo dos Arquivos

### core/ — Dados, Modelos, Estruturas

| Arquivo | Funcao |
|---|---|
| `searchitem` | Struct com nome, versao, repositorio, descricao, comando de instalacao, tipo (pacman/AUR/flatpak) e status de instalacao. |
| `searchmodel` | Adapta lista de `SearchItem` para `QAbstractListModel` com suporte a exportacao CSV/JSON/TXT. |
| `sortfilterproxymodel` | Filtra por nome/descricao e por tipo (Pacman/AUR/Flatpak/todos). |
| `theme.h` | Define cores Catppuccin Mocha/Latte como constantes para uso no C++. |

### engine/ — Logica de Busca e Execucao

| Arquivo | Funcao |
|---|---|
| `searchengine` | Executa `pacman -Ss`, `paru -Ss` e `flatpak search` via `QProcess`, faz o parsing e devolve listas de `SearchItem`. |
| `searchcoordinator` | Centraliza pesquisas (Pacman + AUR em paralelo via `QtConcurrent`) e gerencia fila de comandos (install/remove/update). |
| `commandrunner` | Executa comandos externos via `QProcess` com suporte a sequencias e emite sinais de progresso/conclusao. |

### ui/ — Widgets da Interface

| Arquivo | Funcao |
|---|---|
| `mainwindow` | Janela frameless com titlebar, barra de busca, lista de resultados, detail panel e console. Contem `main()`. |
| `detailpanel` | Painel lateral com informacoes detalhadas do pacote selecionado e botoes Instalar/Remover. |
| `outputconsole` | Console tipo terminal com codificacao de cores: erro (vermelho), sucesso (verde), info (ciano). |
| `packagedelegate` | Desenho customizado de cada item da lista com nome, versao, repo e botoes de acao. |

### audio/ — Audio

| Arquivo | Funcao |
|---|---|
| `soundmanager` | Singleton que toca sons de sucesso, erro e clique via `QSoundEffect`. |

### Estilos

| Arquivo | Funcao |
|---|---|
| `dark.qss` | Tema escuro Catppuccin Mocha (169 linhas). |
| `light.qss` | Tema claro Catppuccin Latte (169 linhas). |

### Build e Deploy

| Arquivo | Funcao |
|---|---|
| `CMakeLists.txt` | Configuracao CMake com Qt6, C++17, AUTOMOC/AUTORCC. |
| `sounds.qrc` | Vincula WAV e QSS como recursos embedados no binario. |
| `run.sh` | Compila (se necessario) e executa o binario. |
| `pack.sh` | Gera `.tar.gz` do projeto. |
| `install-desktop.sh` | Instala icone SVG + atalho .desktop no menu de aplicacoes. |
