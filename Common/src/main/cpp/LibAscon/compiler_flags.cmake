# CMake compiler flags as used by LibAscon
# License: Creative Commons Zero (CC0) 1.0

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

message(STATUS "C compiler ID: ${CMAKE_C_COMPILER_ID}")

if (MSVC)
    message(STATUS "Using compiler flags for MSVC")
    # Options specific for the Microsoft Visual C++ compiler CL
    # Activate a million warnings to have the cleanest possible code
    string(APPEND CMAKE_C_FLAGS " -Wall")  # Activate all warnings
    # (compared to GCC, this actually turns on ALL of the warnings, not just most)
    string(APPEND CMAKE_C_FLAGS " -Qspectre")  # Let the compiler inject Spectre mitigation code
    string(APPEND CMAKE_C_FLAGS " -wd5045")  # Suppress the warning about Spectre mitigation
    # The problem is that even when the Spectre mitigation flag is enabled, the warning about
    # the mitigation being required still appears, so we have to forcibly disable it.
    string(APPEND CMAKE_C_FLAGS " -wd4996")  # Suppress warning about deprecated stdio functions
    # The warning notifies that the functions should be replaced with the safer C11 alternatives
    # fopen -> fopen_s, fscanf -> fscanf_s etc. Here they are only used in the test framework,
    # not in the Ascon implementation, so they are not critical. They are also used in a safe
    # manner to start with, given that the parsed data is fixed (the test vectors).
    # Finally, not every clib implements them, so we cannot assume the compilation succeeds
    # if we use them. Thus, better deactivated.
    string(APPEND CMAKE_C_FLAGS " -wd4127")  # Suppress warning about constant conditional expr.
    # This warning only pops up in the test suite's checks of the Ascon context and state struct
    # sizes, which ARE constant. The tests are there just as a double-check, an assertion,
    # and must stay, so the warning is disabled.
    string(APPEND CMAKE_C_FLAGS " -wd4710 -wd4711")  # Suppress informational warning about inlining
    # MSVC notifies with a warning when the optimised inlines/does not inline a function that
    # is not/is marked for inlining. The warning is informational, thus disabled.

    # Debug mode
    string(APPEND CMAKE_C_FLAGS_DEBUG " -Od")  # Do not optimise

    # Release mode optimised for speed
    string(APPEND CMAKE_C_FLAGS_RELEASE " -O2")  # Optimise for speed
    string(APPEND CMAKE_C_FLAGS_RELEASE " -WX")  # Warnings as errors

    # Release mode optimised for size
    string(APPEND CMAKE_C_FLAGS_MINSIZEREL " -O1")  # Optimise for size
    string(APPEND CMAKE_C_FLAGS_RELEASE " -WX")  # Warnings as errors
else ()
    message(STATUS "Using compiler flags for GCC and Clang")
    # Options for other compilers (generally GCC and Clang)
    # Activate a million warnings to have the cleanest possible code
    string(APPEND CMAKE_C_FLAGS " -Wall -Wextra -pedantic") # Activate most warnings
    string(APPEND CMAKE_C_FLAGS " -Wconversion")  # Values are implicitly converted
    string(APPEND CMAKE_C_FLAGS " -Wsign-conversion")  # Signed to/from unsigned implicit conversion
    string(APPEND CMAKE_C_FLAGS " -Wdouble-promotion")  # Floats implicitly promoted to doubles
    string(APPEND CMAKE_C_FLAGS " -Wfloat-equal")  # Floats compared exactly instead of approx.
    string(APPEND CMAKE_C_FLAGS " -Wswitch-default")  # Switch-case missing default
    string(APPEND CMAKE_C_FLAGS " -Wswitch-enum")  # Switch-case of an enum not covering all values
    string(APPEND CMAKE_C_FLAGS " -Wuninitialized")  # Usage of uninitialised variable
    string(APPEND CMAKE_C_FLAGS " -Wno-unused-variable")  # Variable never used
    string(APPEND CMAKE_C_FLAGS " -Wpacked")  # Packing of struct not needed
    # NOTE: deactivating padded struct warning: all structs in LibAscon have been inspected and
    # optimised. Some will still inevitably have trailing padding (to reach the aligned address
    # where the next struct in an array of structs would start) and this padding varies on each
    # platform. There is not much to be done anymore.
    #string(APPEND CMAKE_C_FLAGS " -Wpadded")  # Struct contains paddings
    string(APPEND CMAKE_C_FLAGS " -Wshadow")  # Shadowing variable name
    string(APPEND CMAKE_C_FLAGS " -Waggregate-return")  # Returning a struct from a function
    string(APPEND CMAKE_C_FLAGS " -Wformat-security")  # (s/f)printf format string vulnerability
    string(APPEND CMAKE_C_FLAGS " -Wlogical-not-parentheses")  # Unclear boolean expression
    string(APPEND CMAKE_C_FLAGS " -Wmissing-declarations")  #
    string(APPEND CMAKE_C_FLAGS " -Wnull-dereference")  # Potential NULL pointer dereference

    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "gcc")
        string(APPEND CMAKE_C_FLAGS " -Wduplicate-cond")  # Checking same thing twice
        string(APPEND CMAKE_C_FLAGS " -Wjump-misses-init")  # Switch/goto jump skips variable init
    endif()

    # Debug mode
    string(APPEND CMAKE_C_FLAGS_DEBUG " -g3")  # Max debug info
    string(APPEND CMAKE_C_FLAGS_DEBUG " -O0")  # Do not optimise
    string(APPEND CMAKE_C_FLAGS_DEBUG " -coverage")  # Gather code coverage info

    # Release mode optimised for speed
    string(APPEND CMAKE_C_FLAGS_RELEASE " -O3")  # Max optimisation for speed
    string(APPEND CMAKE_C_FLAGS_RELEASE " -Werror")  # Warnings as errors
    string(APPEND CMAKE_C_FLAGS_RELEASE " -fomit-frame-pointer")
    string(APPEND CMAKE_C_FLAGS_RELEASE " -march=native")
    string(APPEND CMAKE_C_FLAGS_RELEASE " -mtune=native")

    # Release mode optimised for size
    string(APPEND CMAKE_C_FLAGS_MINSIZEREL " -Os")  # Optimise for size
    string(APPEND CMAKE_C_FLAGS_MINSIZEREL " -fomit-frame-pointer")
    string(APPEND CMAKE_C_FLAGS_MINSIZEREL " -march=native")
    string(APPEND CMAKE_C_FLAGS_MINSIZEREL " -mtune=native")
    string(APPEND CMAKE_C_FLAGS_MINSIZEREL " -funroll-loops")
endif ()
# Options for all compilers
string(APPEND CMAKE_C_FLAGS " ")
string(APPEND CMAKE_C_FLAGS_DEBUG " -DDEBUG")
string(APPEND CMAKE_C_FLAGS_RELEASE " -DRELEASE")
string(APPEND CMAKE_C_FLAGS_MINSIZEREL " -DMINSIZEREL")

# Append sanitiser flags on non-Windows systems
if (NOT WIN32 AND NOT CYGWIN AND NOT MSYS)
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"
            OR "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
        string(APPEND CMAKE_C_FLAGS_DEBUG " -static-libsan")  # Note: san, not Asan
    else()  # GCC
        string(APPEND CMAKE_C_FLAGS_DEBUG " -static-libasan")  # Note: Asan, not san
    endif()
    string(APPEND CMAKE_C_FLAGS_DEBUG " -fsanitize=address,undefined")
    string(APPEND CMAKE_C_FLAGS_DEBUG " -fno-omit-frame-pointer")
    string(APPEND CMAKE_C_FLAGS_DEBUG " -mno-omit-leaf-frame-pointer")
endif ()
