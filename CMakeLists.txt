cmake_minimum_required(VERSION 3.20)

project(KeyVault VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(keyvault_core
    src/key_value_store.cpp
)

target_include_directories(keyvault_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(keyvault_core
    PUBLIC Threads::Threads
)

add_executable(keyvault
    src/main.cpp
)

target_link_libraries(keyvault
    PRIVATE keyvault_core
)

include(CTest)

if(BUILD_TESTING)
    add_executable(keyvault_tests
        tests/key_value_store_test.cpp
    )

    target_link_libraries(keyvault_tests
        PRIVATE keyvault_core
    )

    add_test(
        NAME keyvault_tests
        COMMAND keyvault_tests
    )
endif()

if(MSVC)
    target_compile_options(
        keyvault_core PRIVATE /W4
    )

    target_compile_options(
        keyvault PRIVATE /W4
    )

    if(BUILD_TESTING)
        target_compile_options(
            keyvault_tests PRIVATE /W4
        )
    endif()
else()
    target_compile_options(
        keyvault_core
        PRIVATE -Wall -Wextra -Wpedantic
    )

    target_compile_options(
        keyvault
        PRIVATE -Wall -Wextra -Wpedantic
    )

    if(BUILD_TESTING)
        target_compile_options(
            keyvault_tests
            PRIVATE -Wall -Wextra -Wpedantic
        )
    endif()
endif()