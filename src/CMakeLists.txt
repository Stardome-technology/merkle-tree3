cmake_minimum_required(VERSION 3.10)
project(MerkleTreeC VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(WITH_CBOR "Enable CBOR serialization support" ON)

# Find dependencies
if(WITH_CBOR)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CBOR REQUIRED libcbor)
    if(CBOR_FOUND)
        message(STATUS "Found libcbor: ${CBOR_VERSION}")
        add_definitions(-DWITH_CBOR)
    else()
        message(WARNING "libcbor not found, CBOR support disabled")
        set(WITH_CBOR OFF)
    endif()
endif()

# Compiler flags
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Od /Zi")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2")
    endif()
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
    endif()
endif()

# Source files
set(MERKLE_TREE_SOURCES
    merkle_tree.c
)

set(MERKLE_TREE_HEADERS
    merkle_tree.h
)

# Add CBOR sources if enabled
if(WITH_CBOR)
    list(APPEND MERKLE_TREE_SOURCES merkle_tree_cbor.c)
    list(APPEND MERKLE_TREE_HEADERS merkle_tree_cbor.h)
endif()

# Create static library
add_library(merkletree STATIC ${MERKLE_TREE_SOURCES})
target_include_directories(merkletree PUBLIC .)

# Link CBOR if enabled
if(WITH_CBOR)
    target_link_libraries(merkletree ${CBOR_LIBRARIES})
    target_include_directories(merkletree PRIVATE ${CBOR_INCLUDE_DIRS})
    target_compile_options(merkletree PRIVATE ${CBOR_CFLAGS_OTHER})
endif()

# Create shared library (optional)
add_library(merkletree_shared SHARED ${MERKLE_TREE_SOURCES})
target_include_directories(merkletree_shared PUBLIC .)
set_target_properties(merkletree_shared PROPERTIES OUTPUT_NAME merkletree)

# Link CBOR to shared library if enabled
if(WITH_CBOR)
    target_link_libraries(merkletree_shared ${CBOR_LIBRARIES})
    target_include_directories(merkletree_shared PRIVATE ${CBOR_INCLUDE_DIRS})
    target_compile_options(merkletree_shared PRIVATE ${CBOR_CFLAGS_OTHER})
endif()

# Test executable
add_executable(test_merkle_tree test_merkle_tree.c)
target_link_libraries(test_merkle_tree merkletree)

# Enable testing
enable_testing()
add_test(NAME merkle_tree_tests COMMAND test_merkle_tree)

# Installation
install(TARGETS merkletree merkletree_shared
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${MERKLE_TREE_HEADERS}
    DESTINATION include/merkletree
)

# Package configuration
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MerkleTreeCConfig.cmake"
    INSTALL_DESTINATION lib/cmake/MerkleTreeC
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MerkleTreeCConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MerkleTreeCConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MerkleTreeCConfigVersion.cmake"
    DESTINATION lib/cmake/MerkleTreeC
)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Compiler flags: ${CMAKE_C_FLAGS}")
message(STATUS "CBOR support: ${WITH_CBOR}")
if(WITH_CBOR)
    message(STATUS "libcbor version: ${CBOR_VERSION}")
endif()