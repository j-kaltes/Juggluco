LibAscon - Lightweight Authenticated Encryption & Hashing
================================================================================

[![Build Status Master](https://github.com/TheMatjaz/LibAscon/actions/workflows/build_master.yml/badge.svg)](https://github.com/TheMatjaz/LibAscon/actions/workflows/build_master.yml)
[![Build Status Develop](https://github.com/TheMatjaz/LibAscon/actions/workflows/build_develop.yml/badge.svg)](https://github.com/TheMatjaz/LibAscon/actions/workflows/build_develop.yml)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/TheMatjaz/LibAscon)](https://github.com/TheMatjaz/LibAscon/releases/latest)
[![GitHub](https://img.shields.io/github/license/TheMatjaz/LibAscon)](https://github.com/TheMatjaz/LibAscon/blob/master/LICENSE.md)

LibAscon is an ISO C99/C11 cryptographic library wrapping the
[reference C implementation](https://github.com/ascon/ascon-c)
of the Ascon family of lightweight authenticated encryption schemes with
associated data (AEAD) and hashing functions, but it also includes
Init-Update-Final processing and variable tag length. Heavily tested
and ready for embedded systems!


### Disclaimer

This is not a security-hardened implementation, just a simple one focused
mostly on usability, portability and high(er) set of features
There is no added protection against side-channel
attacks other than what the Ascon algorithm itself provides by design.


Features
----------------------------------------

LibAscon provides:

- 3 AEAD ciphers:
  - Ascon128 v1.2 (128 bit key, 64 bit rate)
  - Ascon128a v1.2 (128 bit key, 128 bit rate)
  - Ascon80pq v1.2 (160 bit key, 64 bit rate)

- 2 hashing functions
  - Ascon-Hash v1.2 (fixed-length output)
  - Ascon-XOF v1.2 (variable-length output)

- **Online processing** (**Init-Update-Final** paradigm) for hashing and
  encryption/decryption. This means that the data can be processed one
  chunk at the time. Useful when operating on large amounts of data that are
  not available contiguously in memory, e.g. a too-large file or data in
  transmission.

- **Offline processing** (whole data contiguous in memory) is also
  available with a simple wrapper.

- **Variable tag length** for authenticated encryption: can generate any tag
  length. Of course at least 16 bytes (128 bits) is recommended.

  Note: the tag bytes above 16 bytes are an **extension** of the original Ascon
  algorithm using the same sponge squeezing technique as for the XOF;

- Encryption/decryption can be performed **in-place**, without the need of a
  second output buffer.

- AEAD tag may be provided to a **separate location**, i.e. not concatenated to
  the ciphertext.

- Same performance as the original C implementation in _Release_ mode,
  about 2x slower in _MinSizeRel_ mode.

- A **[heavily documented](https://thematjaz.github.io/LibAscon/)
  developer-friendly API**, making it easier to compile and add to your project,
  both through static and dynamic inclusion.

- Tested with **100% line coverage**, with CI running on Linux, macOS and
  Windows with GCC, Clang and CL (MSVC).


Usage example
----------------------------------------

### En/decrypting using Ascon128's Init-Update-Final API

```c
#include "ascon.h"
// Initialisation
// We need the key and the nonce, both 128 bits.
// Note: Ascon80pq uses longer keys
const uint8_t secret_key[ASCON_AEAD128_KEY_LEN] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6
};
const uint8_t unique_nonce[ASCON_AEAD_NONCE_LEN] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6
};
ascon_aead_ctx_t ctx;
ascon_aead128_init(&ctx, secret_key, unique_nonce);

// Now we feed any associated data into the cipher first
// Our data is fragmented into 2 parts, so we feed one at the time.
const char associated_data_pt1[] = "2 messages will foll";
const char associated_data_pt2[] = "ow, but they are both secret.";
ascon_aead128_assoc_data_update(&ctx, (uint8_t*) associated_data_pt1, strlen
        (associated_data_pt1));
ascon_aead128_assoc_data_update(&ctx, (uint8_t*) associated_data_pt2, strlen
        (associated_data_pt2));

// Next, we feed the plaintext, which is also fragmented in 2 parts.
const char plaintext_pt1[] = "Hello, I'm a secret mes";
const char plaintext_pt2[] = "sage and I should be encrypted!";
uint8_t buffer[100];
// The ciphertext is generated block-wise, so we need the return value
// to know how to offset the pointer to where the next ciphertext
// part should be written.
size_t ciphertext_len = 0;
ciphertext_len += ascon_aead128_encrypt_update(
        &ctx, buffer + ciphertext_len,
        (uint8_t*) plaintext_pt1, strlen(plaintext_pt1));
ciphertext_len += ascon_aead128_encrypt_update(
        &ctx, buffer + ciphertext_len,
        (uint8_t*) plaintext_pt2, strlen(plaintext_pt2));

// Finally, we wrap up the encryption and generate the tag.
// There may still be some trailing ciphertext to be produced.
// The tag length can be specified. ASCON_AEAD_TAG_MIN_SECURE_LEN is
// the minimum recommended (128 b)
uint8_t tag[ASCON_AEAD_TAG_MIN_SECURE_LEN];
ciphertext_len += ascon_aead128_encrypt_final(
        &ctx, buffer + ciphertext_len,
        tag, sizeof(tag));
// The final function zeroes out the context automatically.
// Now the buffer contains our ciphertext, long ciphertext_len bytes.

// Now we can decrypt, reusing the same key, nonce and associated data
ascon_aead128_init(&ctx, secret_key, unique_nonce);
ascon_aead128_assoc_data_update(&ctx, (uint8_t*) associated_data_pt1,
                                strlen(associated_data_pt1));
ascon_aead128_assoc_data_update(&ctx, (uint8_t*) associated_data_pt2,
                                strlen(associated_data_pt2));
// This time, we perform the decryption in-place, in the same buffer
// where the ciphertext it: to do so, we pass the same pointer for
// plaintext and ciphertext.
size_t plaintext_len = 0;
plaintext_len += ascon_aead128_decrypt_update(
        &ctx, buffer,
        buffer, ciphertext_len);
// The final decryption step automatically checks the tag
bool is_tag_valid = false;
plaintext_len += ascon_aead128_decrypt_final(
        &ctx, buffer + plaintext_len,
        &is_tag_valid, tag, sizeof(tag));
// The final function zeroes out the context automatically.
buffer[plaintext_len] = '\0'; // Null terminated, because it's text
printf("Decrypted msg: %s, tag is valid: %d\n", buffer, is_tag_valid);
// The macros ASCON_TAG_OK=true and ASCON_TAG_INVALID=false are also
// available if you prefer them over booleans for is_tag_valid.
```

### Encrypting using Ascon128's offline API for contiguous data

```c
// We need the key and the nonce, both 128 bits.
// Note: Ascon80pq uses longer keys
const uint8_t secret_key[ASCON_AEAD128_KEY_LEN] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6
};
const uint8_t unique_nonce[ASCON_AEAD_NONCE_LEN] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6
};

// For the "offline" operation, we need all data to be already contiguous
// in memory. The operation is just one Ascon function call for the user.
const char associated_data[] = "1 contiguous message will follow.";
const char plaintext[] = "Hello, I'm a secret message and I should be encrypted!";

uint8_t buffer[100];
uint8_t tag[42];
// To showcase the LibAscon extensions, we will generate an arbitrary-length
// tag. Here is 42 bytes, but shorter-than-default (16) tags are also
// possible e.g. 12 bytes for systems with heavy limitations.
ascon_aead128_encrypt(buffer,
                      tag,
                      secret_key,
                      unique_nonce,
                      (uint8_t*) associated_data,
                      (uint8_t*) plaintext,
                      strlen(associated_data),
                      strlen(plaintext),
                      sizeof(tag));
// The function zeroes out the context automatically.

// The decryption looks almost the same. Just for fun, we will again
// reuse the same buffer where the ciphertext is to write the plaintext into.
bool is_tag_valid = ascon_aead128_decrypt(buffer, // Output plaintext
                                          secret_key,
                                          unique_nonce,
                                          (uint8_t*) associated_data,
                                          (uint8_t*) buffer, // Input ciphertext,
                                          tag, // Expected tag the ciphertext comes with
                                          strlen(associated_data),
                                          strlen(plaintext),
                                          sizeof(tag));
// This time we get a boolean as a return value, which is true when
// the tag is OK. To avoid confusion, it can also be compared to
// two handy macros
if (is_tag_valid == ASCON_TAG_OK) {
    puts("Correct decryption!");
} else { // ASCON_TAG_INVALID
    puts("Something went wrong!");
}
// The function zeroes out the context automatically.
```

### Keyed hashing using Ascon-XOF's Init-Update-Final API

```c
// We want to authenticate a message with a keyed hash and transmit it.
const uint8_t secret_key[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6
};
const char message_pt1[] = "Hello, I'm some data we need the digest of";
const char message_pt2[] = " but I'm very long and I was not transmit";
const char message_pt3[] = "ted in one contiguous block.";

// Preparing the Ascon-XOF function. The XOF allows arbitrary digest size,
// while Ascon-Hash (ascon_hash_init) has a fixed digest length.
// NOTE: Ascon-Hash and Ascon-XOF produce different outputs!
ascon_hash_ctx_t ctx;
ascon_hash_xof_init(&ctx);

// If we want a Message Authentication Code that also proves authenticity,
// not only integrity of the message, we can simply prepend the key to the
// message to hash, just like we would for SHA-3. Length extension attacks
// are not a problem for Ascon-Hash/XOF, so the HMAC scheme is not needed.
ascon_hash_xof_update(&ctx, (uint8_t*) secret_key, sizeof(secret_key));

// Feeding chunk by chunk
ascon_hash_xof_update(&ctx, (uint8_t*) message_pt1, strlen(message_pt1));
ascon_hash_xof_update(&ctx, (uint8_t*) message_pt2, strlen(message_pt2));
ascon_hash_xof_update(&ctx, (uint8_t*) message_pt3, strlen(message_pt3));

// Finally, we get a digest of arbitrary length
uint8_t digest[21];  // Choose any length from 0 to SIZE_MAX
ascon_hash_xof_final(&ctx, digest, sizeof(digest));
// The final function zeroes out the context automatically.

// Now let's imagine we transmit the message alongside with the digest.
// The receiver also has the secret key and can easily verify the keyed hash.
ascon_hash_xof_init(&ctx);
ascon_hash_xof_update(&ctx, (uint8_t*) secret_key, sizeof(secret_key));
ascon_hash_xof_update(&ctx, (uint8_t*) message_pt1, strlen(message_pt1));
ascon_hash_xof_update(&ctx, (uint8_t*) message_pt2, strlen(message_pt2));
ascon_hash_xof_update(&ctx, (uint8_t*) message_pt3, strlen(message_pt3));
// A handy function computing the obtained digest and validating it
// against the obtained.
bool is_tag_valid = ascon_hash_xof_final_matches(&ctx, digest, sizeof(digest));
if (is_tag_valid == ASCON_TAG_OK) {
    puts("Correct decryption!");
} else { // ASCON_TAG_INVALID
    puts("Something went wrong!");
}
```

For more examples, check the test suite and how the Ascon API is used in there.


Dependencies of the library
----------------------------------------

Only the C standard library, mostly C99 features:

- `stdint.h`: `uint8_t`, `uint_fast8_t`, `uint64_t`
- `stddef.h`: `size_t`, `NULL`
- `stdbool.h`: `bool`, `true`, `false`

Optional dependency: `assert.h`, for  `assert()`. Used to add runtime checks of
the Ascon API input for debugging (NULL pointers and incorrect order of
function calling). The CMake script uses it only if `assert.h` is found
and only in the debug build type (`CMAKE_BUILD_TYPE=Debug`).
If compiled without the included `CMakeLists.txt`, it's not used unless
`ASCON_INPUT_ASSERTS` is defined at compile time. The assertion function
`ASCON_ASSERT` can also be changed, if `assert()` is not available.



FAQ
----------------------------------------

- **Q**: May I use this library for-

  **A**: YES! :D The library (and also the Ascon reference implementation)
  is released under the Creative Commons Zero (CC0) license which basically(*)
  means you can do whatever you want with it. Commercial purposes, personal
  projects, modifications of it, anything goes.

  (*) for the legally-binding text check the [license file](LICENSE.md).

- **Q**: Where is the documentation?

  **A**: The [library header file `inc/ascon.h`](inc/ascon.h) is documented with
  Doxygen and you can find it
  [compiled here](https://thematjaz.github.io/LibAscon/).

- **Q**: Why should I use Ascon-AEAD instead of, say, AES?

  **A**: Ascon is designed to be lightweight (great for embedded systems) and
  natively supports authenticated encryption of data of any length, while
  AES must be wrapped in an AEAD mode such as AES-GCM, which is well-proven
  but heavier.

- **Q**: Why should I use Ascon-Hash instead of, say, SHA-256?

  **A**: Ascon is designed to be lightweight (great for embedded systems)
  and does not suffer from length-extension attacks as SHA-256, given its
  sponge construction (similarly to what SHA-3 does). Additionally Ascon
  offers also the XOF hashing producing digests of variable length.

- **Q**: Who designed Ascon? Who wrote this library?

  **A**: Check the [Authors file](AUTHORS.md) for details.

- **Q**: I don't trust Ascon.

  **A**: Good. You should not keep your guard up for everything, but Ascon
  has been selected as the primary choice for lightweight authenticated
  encryption in the final portfolio of the
  [CAESAR competition (2014–2019)](https://competitions.cr.yp.to/caesar-submissions.html)
  and is currently competing in the
  [NIST Lightweight Cryptography competition (2019–)](https://csrc.nist.gov/projects/lightweight-cryptography).
  Cryptographers like it and that's a good sign, right?

- **Q**: I don't trust this implementation.

  **A**: Good, again. You can read the source code to see what it
  does to be sure ;) If you find any bugs or possible improvements,
  open a pull request or an issue. I would like to make as clear and as good
  as possible.


Known limitations
----------------------------------------

- There is no architecture-specific optimisation, only a generic portable
  implementation using mostly `uint64_t` data types.


Compiling
----------------------------------------

The project's compilation has been tested with GCC, Clang and CL (MSVC).
Most compiler warnings have already been mitigated, so they hopefully don't
occur on your platform.


### Static source inclusion

The project is relatively small, so you can simply include it into yours
as a Git Subtree, Git Submodule or by simply copy-pasting the `inc` and `src`
folders. Be sure to:

- Add `inc` and `src` to the include folders list
  (the internal header is in `src`).
- Add `src` to the sources folders list.
- Compile.


### Compiling Ascon into all possible targets with CMake

A note if you are compiling on Windows from the command line:

- you will need
  [the environment variables](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160#developer_command_file_locations)
  to use the MSVC toolchain from the command line
- be sure to select the generator `NMake Makefiles` when configuring CMake

Compile an out of source build:

```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release  # On Windows add: -G "NMake Makefiles"
cmake --build . --parallel 8
```

This will build all useful targets:

- static libraries:
  - `asconfull` with full feature set
  - `ascon128` with only Ascon128
  - `ascon128a` with only Ascon128a
  - `ascon80pq` with only Ascon80pq
  - `asconhash` with only Ascon-hash and Ascon-XOF
  - `ascon128hash` with only Ascon128, Ascon-hash and Ascon-XOF
  - `ascon128ahash` with only Ascon128a, Ascon-hash and Ascon-XOF
  - `ascon80pqhash` with only Ascon80pq, Ascon-hash and Ascon-XOF
  for a smaller build result when not all features are needed
- `ascon` a shared library (`.dll` or `.dylib` or `.so`) with full feature set
  (like `asconfull`, but shared)
- `testascon` a test runner executable , which test all features of the
  static library
- `testasconshared` a test runner executable , which test all features
  of the shared library

Doxygen (if installed) is built separately to avoid recompiling it for any
library change:
```
cmake --build . --target ascon_doxygen
```

To compile only a single target, for example `ascon80pq`, run
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target ascon80pq
```

To compile with the optimisation for size, use the
`-DCMAKE_BUILD_TYPE=MinSizeRel` flag instead.

To run the test executables, call `ctest` in the build directory.
