# CMake configuration to build LibAscon
# License: Creative Commons Zero (CC0) 1.0
#
# For example usage: see README.md
set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.9)
project(LibAscon
        VERSION 1.1.1
        LANGUAGES C
        DESCRIPTION
        "Lightweight Authenticated Encryption & Hashing, \
        also with Init-Update-Final paradigm.")

# Unless specified, by default create Release builds
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# -----------------------------------------------------------------------------
# Compiler flags
# -----------------------------------------------------------------------------
include(compiler_flags.cmake)

include(CheckIncludeFile)
CHECK_INCLUDE_FILE("assert.h" ASSERT_H_EXISTS)
if (ASSERT_H_EXISTS)
    message(STATUS "assert.h found. Enabling runtime assertions verifying "
            "the library API inputs using assert() but only in debug mode "
            "for all targets.")
    string(APPEND CMAKE_C_FLAGS_DEBUG " -DASCON_INPUT_ASSERTS")
endif ()


# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
set(LIB_HEADER inc/ascon.h)
set(LIB_SRC_FILES_COMMON
        ${LIB_HEADER}
        src/ascon_internal.h
        src/ascon_permutations.c
        src/ascon_buffering.c)
set(LIB_SRC_FILES_128
        ${LIB_SRC_FILES_COMMON}
        src/ascon_aead_common.c
        src/ascon_aead128.c)
set(LIB_SRC_FILES_128a
        ${LIB_SRC_FILES_COMMON}
        src/ascon_aead_common.c
        src/ascon_aead128a.c)
set(LIB_SRC_FILES_80pq
        ${LIB_SRC_FILES_COMMON}
        src/ascon_aead_common.c
        src/ascon_aead128.c  # AEAD-80pq reuses many AEAD-128 functions
        src/ascon_aead80pq.c
        )
set(LIB_SRC_FILES_HASH
        ${LIB_SRC_FILES_COMMON}
        src/ascon_hash.c)
set(LIB_SRC_FILES_FULL
        ${LIB_SRC_FILES_COMMON}
        ${LIB_SRC_FILES_128}
        ${LIB_SRC_FILES_128a}
        ${LIB_SRC_FILES_80pq}
        ${LIB_SRC_FILES_HASH}
        )
set(TEST_FRAMEWORK
        tst/atto/atto.c
        tst/main.c
        tst/vectors.c)
set(TEST_SRC_FILES
        tst/test_xof.c
        tst/test_hash.c
        tst/test_aead128_enc.c
        tst/test_aead128_dec.c
        tst/test_aead128_inplace.c
        tst/test_aead128_vartaglen.c
        tst/test_aead80pq_enc.c
        tst/test_aead80pq_dec.c
        tst/test_aead80pq_inplace.c
        tst/test_aead80pq_vartaglen.c
        tst/test_aead128a_enc.c
        tst/test_aead128a_dec.c
        tst/test_aead128a_inplace.c
        tst/test_aead128a_vartaglen.c
        tst/test_structs.c
        tst/test_readme_example.c
        )
set(BENCHMARK_SRC_FILES
        tst/benchmark.c)


# -----------------------------------------------------------------------------
# Build targets
# -----------------------------------------------------------------------------
# Copy the library API header into the build directory.
# With this command, if the header changes, it's copied, otherwise not.
configure_file(${LIB_HEADER} ${CMAKE_BINARY_DIR} COPYONLY)

# Static libraries (.a), compiled with all features or only some features
add_library(asconfull STATIC ${LIB_SRC_FILES_FULL})
target_include_directories(asconfull PUBLIC inc/ PRIVATE src/)
set_target_properties(asconfull PROPERTIES
        INTERPROCEDURAL_OPTIMISATION TRUE)
add_library(ascon128 STATIC ${LIB_SRC_FILES_128})
target_include_directories(ascon128 PUBLIC inc/ PRIVATE src/)
add_library(ascon128a STATIC ${LIB_SRC_FILES_128a})
target_include_directories(ascon128a PUBLIC inc/ PRIVATE src/)
add_library(ascon80pq STATIC ${LIB_SRC_FILES_80pq})
target_include_directories(ascon80pq PUBLIC inc/ PRIVATE src/)
add_library(asconhash STATIC ${LIB_SRC_FILES_HASH})
target_include_directories(asconhash PUBLIC inc/ PRIVATE src/)
add_library(ascon128hash STATIC ${LIB_SRC_FILES_128} ${LIB_SRC_FILES_HASH})
target_include_directories(ascon128hash PUBLIC inc/ PRIVATE src/)
add_library(ascon128ahash STATIC ${LIB_SRC_FILES_128a} ${LIB_SRC_FILES_HASH})
target_include_directories(ascon128ahash PUBLIC inc/ PRIVATE src/)
add_library(ascon80pqhash STATIC ${LIB_SRC_FILES_80pq} ${LIB_SRC_FILES_HASH})
target_include_directories(ascon80pqhash PUBLIC inc/ PRIVATE src/)

# Shared library (.dylib / .dll)
# Does not reuse the static library object files, as they are
# recompiled in order to have position-independent code
add_library(ascon SHARED ${LIB_SRC_FILES_FULL})
target_include_directories(ascon PUBLIC inc/ PRIVATE src/)
set_target_properties(ascon PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        INTERPROCEDURAL_OPTIMISATION TRUE)

# Copy test vectors files to build directory.
# They are used by the test runner
add_custom_target(ascon_copytestvectors ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/tst/vectors
        ${CMAKE_CURRENT_BINARY_DIR}/vectors)
# On make clean (cmake --build . --target clean) also remove the copied
# vectors and the copied header file copied by `configure_file()`.
# Given that configure file is not a target, by adding the file here, we
# have a guarantee it will be cleaned.
set_target_properties(ascon_copytestvectors PROPERTIES
        ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/vectors
        ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/ascon.h)

# Test runner executable testing the static library
add_executable(testascon
        ${TEST_FRAMEWORK}
        ${TEST_SRC_FILES})
target_include_directories(testascon
        PUBLIC inc/
        PRIVATE tst/
        PRIVATE tst/atto/)
add_dependencies(testascon asconfull ascon_copytestvectors)
target_link_libraries(testascon PUBLIC asconfull)
set_target_properties(testascon PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        INTERPROCEDURAL_OPTIMISATION TRUE)

# Test runner executable testing the shared library
add_executable(testasconshared
        ${TEST_FRAMEWORK}
        ${TEST_SRC_FILES})
target_include_directories(testasconshared
        PUBLIC inc/
        PRIVATE tst/
        PRIVATE tst/atto/)
add_dependencies(testasconshared ascon ascon_copytestvectors)
target_link_libraries(testasconshared PUBLIC ascon)
set_target_properties(testasconshared PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        INTERPROCEDURAL_OPTIMISATION TRUE)

# Benchmark runner executable, reuse the static library instead of recompiling
# the source files
add_executable(ascon_benchmark
        ${BENCHMARK_SRC_FILES})
target_include_directories(ascon_benchmark PUBLIC inc/)
add_dependencies(ascon_benchmark asconfull)
target_link_libraries(ascon_benchmark PUBLIC asconfull)
set_target_properties(ascon_benchmark PROPERTIES
        EXCLUDE_FROM_ALL TRUE  # Used just for development, not for deployment
        POSITION_INDEPENDENT_CODE ON
        INTERPROCEDURAL_OPTIMISATION TRUE)

# Doxygen documentation builder
find_package(Doxygen)
if (DOXYGEN_FOUND)
    # Cmake's wrapper of Doxygen, constructing a doxyfile from the
    # DOXYGEN_* variables, which are mapped to the Doxygen variables.
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
    set(DOXYGEN_SORT_MEMBER_DOCS NO)
    set(DOXYGEN_IMAGE_PATH ${PROJECT_SOURCE_DIR}/images)
    set(DOXYGEN_ALIASES license="**License:**")
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    set(DOXYGEN_PREDEFINED WIN32 ASCON_INPUT_ASSERTS)
    doxygen_add_docs(ascon_doxygen
            # Do NOT build doxygen on make-all, to avoid polluting the stdout
            # List of input files for Doxygen
            ${PROJECT_SOURCE_DIR}/inc/ascon.h
            ${PROJECT_SOURCE_DIR}/LICENSE.md
            ${PROJECT_SOURCE_DIR}/AUTHORS.md
            ${PROJECT_SOURCE_DIR}/README.md
            ${PROJECT_SOURCE_DIR}/CHANGELOG.md)
else (DOXYGEN_FOUND)
    message(WARNING "Doxygen not found. Cannot generate documentation.")
endif (DOXYGEN_FOUND)

enable_testing()
add_test(NAME testascon COMMAND ./testascon)
add_test(NAME testasconshared COMMAND ./testasconshared)
