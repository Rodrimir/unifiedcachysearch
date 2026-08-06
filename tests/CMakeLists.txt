set(TEST_SOURCES
    test_search.cpp
    ${CMAKE_SOURCE_DIR}/src/core/command.cpp
    ${CMAKE_SOURCE_DIR}/src/core/exportservice.cpp
    ${CMAKE_SOURCE_DIR}/src/core/searchmodel.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/commandbuilder.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/packageparser.cpp
)

add_executable(test_search ${TEST_SOURCES})

target_link_libraries(test_search PRIVATE
    Qt6::Core
    Qt6::Test
)

target_compile_options(test_search PRIVATE -Wall -Wextra)

target_include_directories(test_search PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME test_search COMMAND test_search)
