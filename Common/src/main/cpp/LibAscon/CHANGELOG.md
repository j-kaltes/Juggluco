Changelog
===============================================================================

All notable changes to this project will be documented in this file.

The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

*******************************************************************************

[1.1.1] - 2021-05-17
----------------------------------------

Compilation fixes for some GCC versions, added `ascon_` to some internal target
names to avoid name clashing.


### Fixed

- Avoid type-punning when clearing the context and when comparing the
  expected and computed tag/digest, which may cause compilation errors
  with `-Werror=strict-aliasing` in some GCC versions.
- Renamed some build targets to avoid name collisions when including the whole
  LibAscon project into another CMake project with `add_subdirectory()`
  - `doxygen` -> `ascon_doxygen`
  - `benchmark` -> `ascon_benchmark`
  - `copytestvectors` -> `ascon_copytestvectors`
  As these are all internal targets, it should hopefully not affect the
  library user.



[1.1.0] - 2021-05-11
----------------------------------------

New hashing functions comparing the expected and computed digest,
support for virtually any tag and digest size, removed dependencies `malloc.h`
and `string.h`, better context cleanup, optional asserts validating the
library input.


### Added

- 4 new hash functions that compute the digest of a message and compare it with
  the expected one, informing the user if they match. This is done with
  constant stack memory usage, like the AEAD functions now validate the tags
  too. The functions need the data and the expected digest as input,
  providing a boolean as output, indicating whether the digest matches or not.
  - `ascon_hash_matches()` and `ascon_hash_xof_matches()` to validate the
    digest of a contiguous message.
  - `ascon_hash_final_matches()` and `ascon_hash_xof_final_matches()` to
    validate the digest as a last step of the Init-Update-Final process,
    removing the need from the user to call `ascon_hash_final()` or
    `ascon_hash_xof_final()` and run `memcmp()` on the just calculated digest.

- **Optional** runtime asserts to validate the argument of the
  library API functions, mostly checking for NULL pointers and correct
  order of calling of the Init-Update-Final functions.
  - Suggested use only in Debug mode.
  - Uses `assert.h` by default, but can be overridden by defining
    `ASCON_ASSERT` at compile time too.
  - In CMake script it's enabled **only** for Debug builds and **only**
    if `assert.h`  is available.
  - **Disabled** by default if compiling the library by other means
    (e.g. custom makefile).

- 2 new example usages in the Readme:
  - Offline encryption/decryption of contiguous data.
  - Hashing functions, including new digest-comparison functions.


### Changed

- Library internals (not impacting API):
  - The AEAD tag validation is not performed one chunk of 8 bytes at the time
    rather than generating the whole contiguous tag from the user-given data and
    comparing it in its entirety (`memcmp()`) with the user-given tag.
    This implies that tag lengths don't have a physical limitation anymore
    (previously tag lengths > 64 bytes were discouraged).
  - Renamed state variable `ascon_bufstate_t.assoc_data_state`
    to `ascon_bufstate_t.flow_state`.
  - Enlarged state enum `ascon_flow_t`.
  - Renamed `const uint8_t* tag` parameter in AEAD function to `expected_tag`
    to emphasise that is the one that comes next to the ciphertext.
    It's length is now similarly `expected_tag_len`.


### Removed

- Dependency `malloc.h`: is not required on Windows anymore, as we don't
  allocate the whole expected tag on the stack anymore: a small 8 byte buffer
  is used instead.

- Dependency `string.h`: due to `memcmp()` (see Changed section) and
  `memset()`/`memset_s()` (see Fixed section) not being used anymore, the
  library is not used.


### Fixed

- The clearing of the context, both for AEAD and hash functions is performed
  without loops or `memset()`/`memset_s()`, but by setting the (not so many)
  context fields one by one to 0 using volatile pointer dereferencing to
  improve the chances of the optimiser not removing the cleanup section.

- CMake fixes:
  - `clean` target now removes `ascon.h` from the build directory.
  - Better copying of the test vectors to the build directory: use a custom
    target, set it as a dependency to `testascon` and `testasconshared` to
    avoid issues on some systems.
  - Building with CMake should now work properly when using LibAscon in a
    Git Submodule.

- Small fixes in the hash/XOF function tests.



[1.0.3] - 2021-05-08
----------------------------------------

Support for compilation on Windows with MSVC, other CMake improvements.

Thanks to [mataron](https://github.com/mataron) for providing the initial
fixes for MSVC!


### Added

- Enable `ctest` command to run the test executables.
- Added `.editorconfig` for a portable text editing configuration.


### Fixed

- All `tag_len` parameters in the library API now are of type `size_t`
  instead of `uint8_t` so that `sizeof()` can be used on the tag buffers.
  As the tags are internally allocated on the stack, the lengths should not
  be excessive (e.g. anything above 64 B = 512 bit is already a lot for
  security purposes).

- LibAscon now successfully compiles with CL (MSVC) on Windows:
  - Fixed errors due to inlining of static function into exposed functions.
  - Fixed errors at link time as the linker did not find the library's public
    API functions: now a `ASCON_API` macro is set to export their symbols
    properly (does nothing on any other OS) with `__declspec(dllexport)`.
  - Use `_malloca()` and `_freea()` to declare arrays on the stack without
    a constant length at compile-time.
  - Fixed a variety of compiler warnings and errors for the test suite code
    and benchmark executable, including paddings, Spectre mitigations,
    inlining notifications, incorrect macro checking.

- Moved compiler flag settings to a separate CMake file: `compiler_flags.cmake`
  - Improved support for GCC vs. Clang differences in the compiler flags

- Replaced Travis CI with GitHub Actions CI:
  - Support for MSVC compilation using CL on Windows



[1.0.2] - 2021-04-30
----------------------------------------

CMake and build process improvements, 4 new targets, minor fix to avoid unwanted
compiler optimisations.


### Added

- Add static build targets `ascon128hash`, `ascon128ahash`, `ascon80pqhash`
  which compile to static libraries with the indicated AEAD cipher and
  the Ascon-Hash/Xof functions. Useful to avoid setting manual compile targets
  when only one cipher and the hash functions are needed.
- Add test runner which tests the shared library build target `testasconshared`
  to check that everything works also with dynamic linking.


### Fixed

- Prefer `memset_s` when available to clear the context, as `memset` may be
  optimised out by the compiler, while `memset_s` is guaranteed to always
  execute.
- Improved `CMakeLists.txt`:
  - Bump minimum CMake version to 3.9 to use the
    `INTERPROCEDURAL_OPTIMISATION` property (aka Link time optimisation)
    on just the targets that need it.
  - Remove `-flto` flag, it may cause compiling issues in some cases (breaking
    the Travis CI build, for one), prefer CMake's abstraction as per point
    above.
  - List explicit include directories for each target.
  - Add explicit dependencies to each target that has some.
- Improved Travis CI:
  - Enable parallel make-all.
  - Install MSYS2 and use GCC on Windows to compile properly.



[1.0.1] - 2020-05-30
----------------------------------------

Fixed slowdowns - now as fast as reference implementation, 100% test coverage.


### Fixed

- Fixed 2x slowdown compared to original reference implementation by
  unrolling loops in `ascon_permutation_[a12|b8|b6]`. Apparently the
  compiler does not do that automatically, even when requested with
  `-funroll-loops`.
  This brings LibAscon to the same performance as the reference implementation,
  when compiled in _Release_ mode.

- When building in _MinSizeRel_ mode (`-DCMAKE_BUILD_TYPE=MinSizeRel`), the core
  round and permutation functions are not hinted to be inlined by the compiled,
  thus the library takes slightly less space.

- Replaced rewritten benchmark runner with original one (copy-pasted and
  slightly changed). Apparently the rewritten benchmark was about 2x slower.
  Now the benchmark results are comparable between original implementation
  and LibAscon.

- Test coverage reached 100%: removed a dead branch in
  `ascon_aead80pq_decrypt_final()`, which was a copy-paste error.

- Fix a `int` to `uint8` type conversion warning.

- Removed unused internal `log_sponge()` function, making the library slightly
  smaller.

- Add initial Travis-CI script for a few builds. Some are still failing, but the
  reasons seems to be in the system configuration or old compiler versions
  or "linker not found", not in the LibAscon source code.



[1.0.0] - 2020-05-21
----------------------------------------

First stable version with all ciphers.


### Modified

- **Breaking** change from previous versions: removed `total_output_len`
  parameters from the functions
  - `ascon_aead*_encrypt()`
  - `ascon_aead*_decrypt()`
  - `ascon_aead*_encrypt_final()`
  - `ascon_aead*_decrypt_final()`
  and from the `ascon_bufstate_t` struct, making it 8 B smaller.
  Why? TL;DR it's redundant.

  The reasoning is that the user of the first two (offline processing)
  already knows the length of the plaintext/ciphertext; the user of the second
  two obtains the length of the processed chunks as return values so they
  can simply sum the up - and anyhow the user known the length of all the
  chunks provided to the cipher; those could be summed up to. In most of the
  cases the argument was `NULL` in the function usage. For details on how to
  obtain the total length, the example in the Readme should suffice.
- Renamed all files in `src` so they start with `ascon_`.


### Fixed

- Added more tests to cover more branching cases of the online-buffering
  algorithm.
- Removal of some minor warnings after inspection with static analyser
  (`scan-build`) and CLion code inspection tool.
- Typos
- Added missing _Known limitations_ paragraphs to the previous releases
  in this Changelog.


### Known limitations

- Because LibAscon is implemented with reuse of existing functions in mind,
  in order to spare on code size and with the Init-Update-Digest paradigm,
  which has some internal buffering, the cipher is about **4x slower** than the
  [reference implementation (`ref`)](https://github.com/ascon/ascon-c).
- There is no architecture-specific optimisation, only a generic portable
  implementation using mostly `uint64_t` data types.



[0.4.0] - 2020-05-20
----------------------------------------

Added Ascon80pq cipher, example in Readme.


### Added

- `ascon_aead128a_*` functions, working exactly as the `aead128` versions.
  Internally they absorb the data with a double rate.
- Example encryption and decrpytion code into Readme.


### Removed

- Macros to exclude some parts of the library from the previous version,
  as they only complicate the building process. It's easier to exclude some
  source files from the build, now that they are better organised.
  - `ASCON_COMPILE_AEAD128`
  - `ASCON_COMPILE_AEAD128a`
  - `ASCON_COMPILE_AEAD80pq`
  -` ASCON_COMPILE_HASH`


### Known limitations

- Because LibAscon is implemented with reuse of existing functions in mind,
  in order to spare on code size and with the Init-Update-Digest paradigm,
  which has some internal buffering, the cipher is about **4x slower** than the
  [reference implementation (`ref`)](https://github.com/ascon/ascon-c).
- There is no architecture-specific optimisation, only a generic portable
  implementation using mostly `uint64_t` data types.



[0.3.0] - 2020-05-20
----------------------------------------

Added Ascon128a cipher and macros to exclude some parts of the library.


### Added

- `ascon_aead128a_*` functions, working exactly as the `aead128` versions.
  Internally they absorb the data with a double rate.
- Added macros to exclude some parts of the library if they are not needed
  for a smaller build
  - `ASCON_COMPILE_AEAD128`
  - `ASCON_COMPILE_AEAD128a`
  - `ASCON_COMPILE_AEAD80pq` (which is not included in the lib yet)
  -` ASCON_COMPILE_HASH`


### Known limitations

- Because LibAscon is implemented with reuse of existing functions in mind,
  in order to spare on code size and with the Init-Update-Digest paradigm,
  which has some internal buffering, the cipher is about **4x slower** than the
  [reference implementation (`ref`)](https://github.com/ascon/ascon-c).
- There is no architecture-specific optimisation, only a generic portable
  implementation using mostly `uint64_t` data types.
- The only AEAD algorithms implemented are Ascon128 and Ascon128a. Ascon80pq is
  still to be done.



[0.2.0] - 2020-05-17
----------------------------------------

Variable tags length, secure context cleanup, minor QOL improvements.


### Added

- `ascon_aead128_cleanup()` and `ascon_hash_cleanup()` to securely cleanup
  the context in case the online processing is not terminated with the
  final function.


### Changed

- `ascon_aead128_encrypt()`, `ascon_aead128_encrypt_final()`,
  `ascon_aead128_decrypt()`, `ascon_aead128_decrypt_final()`
  have a **new parameter**: `tag_len`. The user can specify the length of the
  tag to generate and verify respectively. The value can be any value in
  [0, 255] bytes. At least 16 is of course recommended.

  Note: the tag bytes above 16 B are an extension of the original Ascon
  algorithm using the same sponge squeezing technique as for the XOF.

- Replace `ascon_tag_validity_t` enum with simpler `bool` from `stdbool.h`
  indicating "is tag valid?": `true` for valid, `false` otherwise. It's one
  less datatype to handle. Macros `ASCON_TAG_OK` and `ASCON_TAG_INVALID`
  are still available.

- Because of the previous point: `srtbool.h` as now a dependency.

- Changed ISO C standard for compilation from C99 to C11, as there are
  no noticeable differences for this codebase.


### Fixed

- Improvements in the Doxygen documentation, especially the security warnings.
- Some warnings about type casting when compiling on GCC for Windows or
  ARM microcontrollers with strict warning settings (`-Wall`, `-Wextra` etc.).
- Variety of typos and linter warnings.
- Minor readme improvements.


### Known limitations

- Same as in v0.1.0



[0.1.0] - 2020-02-11
----------------------------------------

Initial version.


### Added

- AEAD encryption and decryption with Ascon128, both offline and online
- Hashing with Ascon-Hash and Ascon-XOF, both offline and online
- Test suite checking against test vectors
- Wrapping everything with single extensively documented header file


### Known limitations

- At the moment the library is **optimised for size**, making the cipher
  about 4x slower than the
  [reference implementation](https://github.com/ascon/ascon-c).
  On the other hand the size of the whole library is about 2x smaller than the
  AEAD-Ascon128 implementation alone, while the library includes online
  processing, Ascon-Hash and Ascon-XOF.
- There is no architecture-specific optimisation yet, only a generic portable
  C implementation using mostly `uint64_t` data types.
- The only AEAD algorithm implemented is the Ascon128 AEAD. The Ascon128a and
  Ascon80pq are still to be done.
