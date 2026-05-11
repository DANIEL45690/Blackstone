cmake_minimum_required(VERSION 3.16)
project(P2PTokenSystem VERSION 4.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    set(PLATFORM_LIBS ws2_32)
endif()

file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(p2p_token ${SOURCES} ${HEADERS})
target_include_directories(p2p_token PRIVATE src)

target_link_libraries(p2p_token ${PLATFORM_LIBS})

if(MSVC)
    target_compile_options(p2p_token PRIVATE /O2 /arch:AVX2 /openmp /GL /W3)
    target_link_options(p2p_token PRIVATE /LTCG)
else()
    target_compile_options(p2p_token PRIVATE -O3 -march=native -mtune=native -flto -Wall -Wextra)
    target_link_options(p2p_token PRIVATE -flto)
endif()

set_target_properties(p2p_token PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)