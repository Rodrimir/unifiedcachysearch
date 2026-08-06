# Estrutura do Projeto — CachyOS Unified Search

Busca unificada de pacotes Pacman, AUR e Flatpak numa única interface Qt6.

```
unifiedcachysearch/
|
|-- CMakeLists.txt               # Build (Qt6, C++17, -Wall -Wextra)
|-- sounds.qrc                   # Recursos: sons WAV + folhas de estilo QSS
|-- run.sh                       # Compila (Release) e executa
|-- make-appimage.sh             # Build de producao: gera o AppImage portatil
|-- pack.sh                      # Empacota o codigo-fonte em .tar.gz
|-- install-desktop.sh           # Instala icone + atalho .desktop no menu
|-- assets/icons/cachysearch.svg # Icone (fonte unica, usada pelos 2 scripts)
|-- sounds/                      # success.wav, error.wav, click.wav
|
|-- src/
|   |-- main.cpp
|   |
|   |-- core/                    # Tipos, modelos e preferencias.
|   |   |                        # Nao executa processos e nao conhece a UI.
|   |   |-- appsettings.h/.cpp         # Chaves e defaults do QSettings (fonte unica)
|   |   |-- command.h/.cpp             # Comando externo: programa + args + env
|   |   |-- searchitem.h               # Dados de um pacote (header-only)
|   |   |-- searchmodel.h/.cpp         # QAbstractItemModel da lista
|   |   |-- sortfilterproxymodel.h/.cpp# Filtro por tipo de repositorio
|   |   |-- repositoryinfo.h           # Nome/icone/cor por tipo de repositorio
|   |   |-- exportservice.h/.cpp       # Exportacao JSON / CSV / TXT
|   |   |-- theme.h                    # Paleta Catppuccin (Mocha / Latte)
|   |
|   |-- engine/                  # Processos externos, parsing e orquestracao.
|   |   |-- processrunner.h/.cpp       # Execucao sincrona + cancelamento seguro
|   |   |-- packageparser.h/.cpp       # Parsing puro das saidas + relevancia
|   |   |-- commandbuilder.h/.cpp      # Monta comandos de instalar/remover
|   |   |-- searchengine.h/.cpp        # Consultas e cache do que esta instalado
|   |   |-- searchcoordinator.h/.cpp   # Buscas paralelas + cross-reference
|   |   |-- commandrunner.h/.cpp       # Execucao assincrona de comandos
|   |
|   |-- ui/                      # Widgets
|   |   |-- mainwindow.h/.cpp          # Janela frameless, modos Busca/Atualizacoes
|   |   |-- detailpanel.h/.cpp         # Painel lateral do pacote selecionado
|   |   |-- outputconsole.h/.cpp       # Console com stdin e modo senha
|   |   |-- packagedelegate.h/.cpp     # Desenho dos itens da lista
|   |   |-- settingsdialog.h/.cpp      # Preferencias
|   |
|   |-- audio/
|   |   |-- soundmanager.h/.cpp        # Singleton de efeitos sonoros
|   |
|   |-- styles/                  # dark.qss (Mocha) e light.qss (Latte)
|
|-- tests/                       # QtTest: 25 casos
    |-- CMakeLists.txt
    |-- test_search.cpp
```

## Camadas e dependencias

O fluxo de dependencia e sempre em uma direcao:

```
ui  ->  engine  ->  core
audio ------------>  core
```

- **core** nao inclui nada de `engine` nem de `ui`.
- **engine** e a unica camada que cria `QProcess`.
- **ui** nunca monta linha de comando: recebe objetos `Command` prontos.

### Por que `Command`

Um comando trafega como programa + lista de argumentos + variaveis de ambiente,
da construcao ate o `QProcess`. Nada passa por shell e nada e reconstruido a
partir de uma string, entao nome de pacote ou remote com caractere especial nao
quebra nem vira dois argumentos. `Command::toString()` existe so para exibir na
UI e nas exportacoes.

### Threading

| Onde | Thread |
|---|---|
| `SearchEngine::search*`, `check*Updates`, `query*PackageInfo` | tarefas do `QtConcurrent` |
| `SearchCoordinator`, `CommandRunner`, toda a `ui` | thread da UI |
| `ProcessRunner::cancelAll()`, `SearchEngine::cancelSearch()` | qualquer thread |

O cancelamento nunca toca no `QProcess` de outra thread: `cancelAll()` incrementa
um contador atomico e cada `run()` em andamento mata o proprio processo. Os
resultados de buscas ja substituidas sao descartados por geracao, sem `wait`
bloqueando a interface.

## Build

| Comando | Para que |
|---|---|
| `./run.sh` | Desenvolvimento: compila em `build/` e executa |
| `./make-appimage.sh` | Producao: AppImage portatil com Qt6 embutido |
| `cmake -B build -DBUILD_TESTS=ON && ctest --test-dir build` | Testes |
| `./install-desktop.sh` | Atalho no menu de aplicacoes |

Os testes cobrem parsing (pacman/AUR/flatpak/updates/info), construcao de
comandos, relevancia, o model e a exportacao — tudo sem depender do que esta
instalado na maquina.
