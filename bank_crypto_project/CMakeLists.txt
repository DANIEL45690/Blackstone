cmake_minimum_required(VERSION 3.14)
project(BankCrypto VERSION 3.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)

if(MSVC)
    add_compile_options(/W4 /WX- /utf-8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB_RECURSE BANK_SOURCES "src/*.cpp")
list(REMOVE_ITEM BANK_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")
list(REMOVE_ITEM BANK_SOURCES "${CMAKE_SOURCE_DIR}/src/test/bank_self_test.cpp")

add_library(bank_crypto STATIC ${BANK_SOURCES})

set_target_properties(bank_crypto PROPERTIES
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(bank_crypto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(bank_demo src/main.cpp)
target_link_libraries(bank_demo bank_crypto)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

install(TARGETS bank_crypto DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
