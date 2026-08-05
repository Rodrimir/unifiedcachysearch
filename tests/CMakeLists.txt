set(TEST_SOURCES
    test_search.cpp
    ${CMAKE_SOURCE_DIR}/src/core/searchitem.cpp
    ${CMAKE_SOURCE_DIR}/src/core/searchmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/searchengine.cpp
)

add_executable(test_search ${TEST_SOURCES})

target_link_libraries(test_search PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Test
)

target_include_directories(test_search PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME test_search COMMAND test_search)
