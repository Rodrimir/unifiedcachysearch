cmake_minimum_required(VERSION 3.20)
project(cachysearch_gui VERSION 0.8.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Network Multimedia)

set(SOURCES
    src/main.cpp
    src/ui/mainwindow.cpp
    src/engine/searchcoordinator.cpp
    src/engine/commandrunner.cpp
    src/core/searchmodel.cpp
    src/core/searchitem.cpp
    src/engine/searchengine.cpp
    src/ui/packagedelegate.cpp
    src/ui/settingsdialog.cpp
    src/ui/detailpanel.cpp
    src/ui/outputconsole.cpp
    src/core/sortfilterproxymodel.cpp
    src/audio/soundmanager.cpp
    src/core/exportservice.cpp
)

set(HEADERS
    src/ui/mainwindow.h
    src/engine/searchcoordinator.h
    src/engine/commandrunner.h
    src/core/searchmodel.h
    src/core/searchitem.h
    src/engine/searchengine.h
    src/ui/packagedelegate.h
    src/ui/settingsdialog.h
    src/ui/detailpanel.h
    src/ui/outputconsole.h
    src/core/sortfilterproxymodel.h
    src/audio/soundmanager.h
    src/core/theme.h
    src/core/exportservice.h
    src/ui/presentationhelper.h
)

add_executable(cachysearch_gui ${SOURCES} ${HEADERS} sounds.qrc)

target_link_libraries(cachysearch_gui PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Multimedia
)

target_compile_definitions(cachysearch_gui PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(cachysearch_gui PRIVATE ${CMAKE_SOURCE_DIR}/src)

option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
