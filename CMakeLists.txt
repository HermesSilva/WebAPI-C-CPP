cmake_minimum_required(VERSION 3.20)
project(TootegaWebAPI VERSION 1.0.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "Windows")
    add_definitions(-DPLATFORM_WINDOWS)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_NAME "Linux")
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
    add_definitions(-DPLATFORM_MACOS)
endif()

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64|AMD64|x86_64|X86_64")
    set(ARCH_NAME "x64")
    add_definitions(-DARCH_X64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|ARM64|aarch64|AARCH64")
    set(ARCH_NAME "ARM64")
    add_definitions(-DARCH_ARM64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|i[3-6]86")
    set(ARCH_NAME "x86")
    add_definitions(-DARCH_X86)
else()
    set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})
endif()

message(STATUS "Building for ${PLATFORM_NAME} ${ARCH_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Version definitions
add_definitions(-DAPI_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
add_definitions(-DAPI_VERSION_MINOR=${PROJECT_VERSION_MINOR})
add_definitions(-DAPI_VERSION_PATCH=${PROJECT_VERSION_PATCH})
add_definitions(-DAPI_VERSION="${PROJECT_VERSION}")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Source files
set(SOURCES
    src/main.cpp
    src/core/server.cpp
    src/core/system_info.cpp
    src/core/jwt.cpp
    src/api/version_controller.cpp
    src/api/docs_controller.cpp
    src/api/browser_controller.cpp
    src/api/auth_controller.cpp
    src/data/database.cpp
)

set(HEADERS
    src/core/server.h
    src/core/system_info.h
    src/core/jwt.h
    src/api/version_controller.h
    src/api/docs_controller.h
    src/api/browser_controller.h
    src/api/auth_controller.h
    src/data/database.h
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Platform-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 odbc32)
elseif(UNIX)
    find_package(Threads REQUIRED)
    # ODBC for database connectivity on Linux
    find_library(ODBC_LIB odbc)
    if(ODBC_LIB)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${ODBC_LIB})
    else()
        message(WARNING "ODBC library not found - database features will not work")
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
endif()

# Compiler-specific flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4 
        /permissive-
        /utf-8
    )
    # Multi-processor compilation
    target_compile_options(${PROJECT_NAME} PRIVATE /MP)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Print configuration summary
message(STATUS "")
message(STATUS "=== Configuration Summary ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "Architecture: ${ARCH_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "=============================")
message(STATUS "")
