cmake_minimum_required(VERSION 3.20)

project(KeyVault VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(keyvault_core
    src/key_value_store.cpp
)

target_include_directories(keyvault_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(keyvault src/main.cpp)

target_link_libraries(keyvault
    PRIVATE keyvault_core
)

if(MSVC)
    target_compile_options(keyvault_core PRIVATE /W4)
    target_compile_options(keyvault PRIVATE /W4)
else()
    target_compile_options(keyvault_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(keyvault PRIVATE -Wall -Wextra -Wpedantic)
endif()
