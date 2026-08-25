

if(${CMAKE_BUILD_TYPE})
else()
option(DEBUG "compile for debugging" OFF)
if(DEBUG)
set(CMAKE_CXX_FLAGS "-DDEBUG=1 -DSCANLOG=1 -UNDEBUG  -DNDK_DEBUG=1  -fPIC " )
set(CMAKE_C_FLAGS "-DDEBUG=1 -DSCANLOG=1 -UNDEBUG  -DNDK_DEBUG=1  -fPIC " )
if(JUGGLUCO_APP)
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -fsanitize=address,undefined " )
set(CMAKE_C_FLAGS "$ ${CMAKE_C_FLAGS} -fsanitize=address,undefined " )
endif()
set(CMAKE_BUILD_TYPE Debug)
else()
if(LOG)
else()
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOLOG=1  ")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNOLOG=1  ")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRELEASE=1 -DNORAWSTREAM=1 -DRELEASEID=1 -DRELEASEDIR=1 ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRELEASE=1 -DNORAWSTREAM=1 -DRELEASEID=1 -DRELEASEDIR=1 ")
set(CMAKE_BUILD_TYPE Release)
endif()
endif()
#cmake_minimum_required(VERSION 3.4.1)
cmake_minimum_required(VERSION 3.13)
project(Glucodata)
#set(MYCFLAGS " -DL3_USE_CRYPTO_DLSYM=1 -g -Wall -Wno-sign-compare -fvisibility=hidden -ffast-math -mcpu=native  -march=native ")
set(MYCFLAGS " -DL3_PROVIDER_SOFT_EMBED_FE_BLOCK -DL3_USE_CRYPTO_DLSYM=1 -g -Wall -Wno-sign-compare -fvisibility=hidden  ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  ${MYCFLAGS}" )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   ${MYCFLAGS} -Wno-vla-cxx-extension -fno-rtti -fexceptions  -DALWAYSPATH=1  ")

if(JUGGLUCO_APP)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -std=gnu++2b   ")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=gnu++26 ")
endif()

include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(sys/prctl.h HAVE_PRCTL)

if(HAVE_PRCTL)
else()
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -DHAVE_NOPRCTL=1 ")
endif()
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-pthread HAVE_PTHREADARG)

set(INCLUDESELF ${CMAKE_CURRENT_SOURCE_DIR})
set(OPENGL_LIB GLESv3)
#set(OPENGL_LIB GLESv2)
set(CURVEDIR curve)
set(NETDIR net)
set(log-lib "log")
set(NANOVG ${CURVEDIR}/nanovg/src)
set(LIBASCON ${INCLUDESELF}/LibAscon)
set(APP_STL c++_static)
#set(APP_STL  c++_shared)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
set(BCRYPTDIR ${INCLUDESELF}/bcrypt/)
set(BCRYPTFILES ${BCRYPTDIR}aes_encrypt.c ${BCRYPTDIR}bcrypt.cpp ${BCRYPTDIR}ccm_mode.c ${BCRYPTDIR}utils.c)

set(EXTRAVIEW ${NETDIR}/libreview/putsensor.cpp ${NETDIR}/libreview/mkrealhistory.cpp ${NETDIR}/libreview/libreview.cpp ${NETDIR}/libreview/newlibre3.cpp ${NETDIR}/libreview/libreview.cpp )

#set(LIBRE3FILES ${INCLUDESELF}/libre3/loadlibs.cpp ${INCLUDESELF}/libre3/bluetooth.cpp ${INCLUDESELF}/libre3/nfc.cpp ${INCLUDESELF}/libre3/libre3_old_policy_bypass.c ) # ${INCLUDESELF}/libre3/libre3_postgen_dump.cpp )
if(libre3)
    set(LIBRE3FILES ${INCLUDESELF}/libre3/loadlibs.cpp ${INCLUDESELF}/libre3/bluetooth.cpp ${INCLUDESELF}/libre3/nfc.cpp  ) # ${INCLUDESELF}/libre3/libre3_postgen_dump.cpp )
endif()


set(ASCONFILES ${LIBASCON}/src/ascon_permutations.c ${LIBASCON}/src/ascon_buffering.c ${LIBASCON}/src/ascon_aead_common.c ${LIBASCON}/src/ascon_aead128a.c)
add_definitions(-DANDROID_STL=c++_static -DDYNLINK -DUSE_MEAL -DNOTIMEOUT  )


set(CHECKER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/nums/checker.cpp")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/misc/checker.cpp")
    set(CHECKER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/misc/checker.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/misc/initcrypto.cpp")
endif()

set(ARJUGGLUCO_INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/${CURVEDIR}/arjugglucotext.cpp")
set(ARJUGGLUCO_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${CURVEDIR}/arjugglucotextgen.cpp")
set_source_files_properties("${ARJUGGLUCO_OUTPUT}" PROPERTIES GENERATED TRUE)





include(ExternalProject)

option(SKIP_RTLPP "Skip building rtlpp_host and use pre-generated arjugglucotextgen.cpp" OFF)

set(RTLPP_DEPENDS "${ARJUGGLUCO_INPUT}")

if(SKIP_RTLPP)
    message(STATUS "SKIP_RTLPP enabled: using pre-generated ${ARJUGGLUCO_OUTPUT}")
elseif(CMAKE_CROSSCOMPILING)
    if(CMAKE_HOST_WIN32)
        set(RTLPP_HOST_EXE_SUFFIX ".exe")
    else()
        set(RTLPP_HOST_EXE_SUFFIX "")
    endif()

    set(RTLPP_HOST_PREFIX     "${CMAKE_CURRENT_BINARY_DIR}/host-tools/rtlpp")
    set(RTLPP_HOST_BINARY_DIR "${RTLPP_HOST_PREFIX}/build")
    set(RTLPP_HOST_BIN_DIR    "${RTLPP_HOST_PREFIX}/bin")
    set(RTLPP_EXECUTABLE      "${RTLPP_HOST_BIN_DIR}/rtlpp${RTLPP_HOST_EXE_SUFFIX}")

    ExternalProject_Add(rtlpp_host
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rtlpp-host"
        BINARY_DIR "${RTLPP_HOST_BINARY_DIR}"
        PREFIX     "${RTLPP_HOST_PREFIX}/prefix"

        DOWNLOAD_COMMAND ""
        UPDATE_COMMAND ""
        PATCH_COMMAND ""

        CMAKE_ARGS
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${RTLPP_HOST_BIN_DIR}"

        BUILD_BYPRODUCTS
            "${RTLPP_EXECUTABLE}"

        INSTALL_COMMAND ""
    )

    set(RTLPP_COMMAND "${RTLPP_EXECUTABLE}")

    list(APPEND RTLPP_DEPENDS
        rtlpp_host
        "${RTLPP_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/rtlpp.cpp"
    )
else()
    set(RTLPP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/rtlpp.cpp")

    find_package(ICU REQUIRED COMPONENTS uc i18n)

    add_executable(rtlpp "${RTLPP_SOURCE}")
    target_link_libraries(rtlpp PRIVATE ICU::uc ICU::i18n)

    set(RTLPP_COMMAND "$<TARGET_FILE:rtlpp>")

    list(APPEND RTLPP_DEPENDS
        rtlpp
        "${RTLPP_SOURCE}"
    )
endif()

if(SKIP_RTLPP)
    add_custom_command(
        OUTPUT "${ARJUGGLUCO_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E true
        DEPENDS "${ARJUGGLUCO_INPUT}"
        COMMENT "Using pre-generated ${ARJUGGLUCO_OUTPUT}"
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT "${ARJUGGLUCO_OUTPUT}"
        COMMAND "${RTLPP_COMMAND}" "${ARJUGGLUCO_INPUT}" "${ARJUGGLUCO_OUTPUT}"
        DEPENDS ${RTLPP_DEPENDS}
        COMMENT "Generating ${ARJUGGLUCO_OUTPUT} from ${ARJUGGLUCO_INPUT}"
        VERBATIM
    )
endif()




add_custom_target(generate_arjugglucotext DEPENDS "${ARJUGGLUCO_OUTPUT}")


set_source_files_properties( ${L3HOOK} PROPERTIES COMPILE_OPTIONS "-marm")


#set(LIBRE3 libre3/process/cryptoextern.c libre3/process/f3f7_dispatch_deobfuscated.c libre3/process/f3f7_dispatch_execute_ref.c libre3/process/f3f7_dispatch_wrappers_ref.c libre3/process/f3f7_object_method_ref.c libre3/process/f3f7_outer_schedule_ref.c libre3/process/libre3_app_process.c libre3/process/libre3_native_export_ref.c libre3/process/libre3_native_export_wb_ref.c libre3/process/libre3_native_wb_primitives_ref.c libre3/process/libre3_object_export_frames_ref.c libre3/process/libre3_process_ref.c libre3/process/libre3_processint6_f3efd5bc_ref.c libre3/process/libre3_processint6_f3fbc5c0_f3fdd33c_bind_ref.c libre3/process/libre3_processint6_f3fbc5c0_live_fc3c70_bind_ref.c libre3/process/libre3_processint6_f3fbc5c0_ref.c libre3/process/libre3_processint6_f3fbc5c0_terminal_bound_ref.c libre3/process/libre3_processint6_f3fc2110_ref.c libre3/process/libre3_processint6_f3fc2304_ref.c libre3/process/libre3_processint6_f3fc3c70_ref.c libre3/process/libre3_processint6_f3fcc2d0_ref.c libre3/process/libre3_processint6_f3fdb57c_ref.c libre3/process/libre3_processint6_f3fdd33c_ref.c libre3/process/libre3_provider_soft.c libre3/process/object_method_known_vectors.c libre3/process/processint2_full_ref.c libre3/process/processint2_ref.c libre3/process/crypto_init.cpp libre3/process/libre3_bar7_fe_block.cpp libre3/process/libre3_bar7_generated_block.cpp)

if(libre3)
set(LIBRE3SRC
    ${INCLUDESELF}/libre3/process/openssl_symbols.c
    ${INCLUDESELF}/libre3/process/libre3_handshake.c
    ${INCLUDESELF}/libre3/process/libre3_security_engine.c
    ${INCLUDESELF}/libre3/process/saved_authorization_record.c
    ${INCLUDESELF}/libre3/process/sensor_security_context.c
    ${INCLUDESELF}/libre3/process/p256_private_scalar_codec.c
    ${INCLUDESELF}/libre3/process/app_private_key_record_decoder.c
    ${INCLUDESELF}/libre3/process/app_private_key_whitebox_machine.c
    ${INCLUDESELF}/libre3/process/app_private_key_instruction_decoder.c
    ${INCLUDESELF}/libre3/process/app_private_key_instruction_executor.c
    ${INCLUDESELF}/libre3/process/app_private_key_instruction_set.c
    ${INCLUDESELF}/libre3/process/app_private_key_decode_schedule.c
    ${INCLUDESELF}/libre3/process/authorization_scalar_state_encoder.c
    ${INCLUDESELF}/libre3/process/p256_scalar_multiply.c
    ${INCLUDESELF}/libre3/process/authorization_round_feedback.c
    ${INCLUDESELF}/libre3/process/authorization_round_tail.c
    ${INCLUDESELF}/libre3/process/authorization_round_mixer.c
    ${INCLUDESELF}/libre3/process/authorization_round_output.c
    ${INCLUDESELF}/libre3/process/p256_coordinate_codec.c
    ${INCLUDESELF}/libre3/process/p256_coordinate_reduction.c
    ${INCLUDESELF}/libre3/process/authorization_round_primary.c
    ${INCLUDESELF}/libre3/process/authorization_round_secondary.c
    ${INCLUDESELF}/libre3/process/authorization_vector_reduction.c
    ${INCLUDESELF}/libre3/process/authorization_vector_feedback.c
    ${INCLUDESELF}/libre3/process/authorization_round_driver.c
    ${INCLUDESELF}/libre3/process/challenge_whitebox_block_cipher.cpp
    ${INCLUDESELF}/libre3/process/openssl_loader.cpp
    ${INCLUDESELF}/libre3/process/authorization_shared_point_frames.c
    ${INCLUDESELF}/libre3/process/authorization_shared_point_tail.c
    ${INCLUDESELF}/libre3/process/authorization_frame_transform.c
    ${INCLUDESELF}/libre3/process/authorization_digest.c
    ${INCLUDESELF}/libre3/process/authorization_finalizer.c
    ${INCLUDESELF}/libre3/process/p256_coordinate_extract.c
    ${INCLUDESELF}/libre3/process/p256_private_scalar_decoder.c
    ${INCLUDESELF}/libre3/process/p256_ephemeral_keypair.c
    ${INCLUDESELF}/libre3/process/challenge_cipher_context.c
)
endif()
add_library( g SHARED  ${LIBRE3SRC} sibionics3/interpretgs3.cpp sibionics3/java.cpp curve/scriptFonts.cpp settings/rtl_label_converter.cpp ${CHECKER_FILE} aidexx/crc.cpp aidexx/initcrypto.cpp  aidexx/java.cpp aidexx/decrypt.cpp aidexx/mkiv.cpp ${NETDIR}/ICE/ContextHTTPS.cpp  ${NETDIR}/ICE/ICE.cpp  ${NETDIR}/ICE/ICE_data.cpp ${NETDIR}/sendcommands.cpp ${NETDIR}/backup.cpp ${NETDIR}/getcommand.cpp ${BCRYPTFILES} meter/java.cpp calibrate/java.cpp  calibrate/calculate.cpp  air/java.cpp   accu/java.cpp  dexcom/java.cpp dexcom/ecJPake.cpp extra/bypassed_dlfcn.c extra/fake_dlfcn.c EverSense.cpp dofirst.cpp settings/exception.cpp prunestream.cpp ${EXTRAVIEW} watch/messages.cpp watch/netinfo.cpp watch/watchvalue.cpp removeoldstates.cpp  ${CURVEDIR}/trjugglucotext.cpp   ${CURVEDIR}/svjugglucotext.cpp    ${CURVEDIR}/zhjugglucotext.cpp  ${CURVEDIR}/ukjugglucotext.cpp  ${CURVEDIR}/bejugglucotext.cpp ${CURVEDIR}/frjugglucotext.cpp ${CURVEDIR}/pljugglucotext.cpp ${CURVEDIR}/ptjugglucotext.cpp  ${CURVEDIR}/jajugglucotext.cpp ${CURVEDIR}/hijugglucotext.cpp ${CURVEDIR}/esjugglucotext.cpp ${CURVEDIR}/uzjugglucotext.cpp ${CURVEDIR}/nljugglucotext.cpp ${CURVEDIR}/dejugglucotext.cpp  ${CURVEDIR}/jugglucotext.cpp  ${CURVEDIR}/itjugglucotext.cpp ${CURVEDIR}/percentile.cpp foods/javabase.cpp ${ASCONFILES} hostJson.cpp backupjava.cpp glucose.cpp ${CURVEDIR}/shownums.cpp g.cpp libre2.cpp jnisub.cpp share/getsockets.cpp share/hooks.cpp wearInt.cpp logjava.cpp share/logs.cpp share/nfcdata.cpp  share/serial.cpp scanstate.cpp statedata.cpp share/timestr.cpp share/savehistory.cpp  ${NANOVG}/nanovg.c  ${CURVEDIR}/search.cpp ${CURVEDIR}/appcurve.cpp ${CURVEDIR}/curve.cpp ${CURVEDIR}/javacurve.cpp export/libreviewexport.cpp export/exports.cpp settings/javasettings.cpp settings/settings.cpp debugclone.cpp ${NETDIR}/watchserver/common.cpp ${NETDIR}/watchserver/getlanguage.cpp ${NETDIR}/watchserver/uploader.cpp  ${NETDIR}/watchserver/uploadtreatment.cpp  ${NETDIR}/watchserver/iob.cpp   ${NETDIR}/watchserver/sslserver.cpp  ${NETDIR}/watchserver/cryptopen.cpp ${NETDIR}/watchserver/watchserver.cpp ${NETDIR}/netstuff.cpp ${NETDIR}/sender.cpp ICEstatus.cpp mirrorstatus.cpp backupinfo.cpp datbackup.cpp nums/numdata.cpp nums/numjava.cpp  meal/mealjava.cpp meal/savemeals.cpp  notes/notesjava.cpp  ${LIBRE3FILES}  ${CURVEDIR}/iwjugglucotext.cpp ${CURVEDIR}/rujugglucotext.cpp  sibionics/handleData.cpp  sibionics/interpret_data.cpp sibionics/eu.cpp sibionics/start.cpp sibionics/process.cpp sibionics/makeWriteCharacter.cpp  ) #  ${CURVEDIR}/iwjugglucotext.cpp 
target_sources(g PRIVATE "${ARJUGGLUCO_OUTPUT}")
add_dependencies(g generate_arjugglucotext)
target_compile_definitions(g PRIVATE  -DUSE_SSL=1 -DANDROID__APP=1  -DJUGGLUCO_APP=1 -DUSE_FREETEXT_NOTE=1)
target_include_directories(g PRIVATE ${NANOVG}  ${INCLUDESELF} ${INCLUDESELF}/share ${INCLUDESELF}/LibAscon/inc ${INCLUDESELF}/include ${INCLUDESELF}/bcrypt)
target_compile_definitions( g  PRIVATE  -DWEAROS_MESSAGES=1 -DDONTSCALESTROKEWIDTH  -DLIBRENUMBERS=1 )

add_link_options(-g  )
target_link_options(g PRIVATE "-Wl,-z,max-page-size=16384")
#target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-Wl,-z,max-page-size=16384")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(NO_SERVER ON CACHE BOOL "Disable server support" FORCE)
set(NO_TESTS  ON CACHE BOOL "Disable tests build" FORCE)

add_subdirectory(libjuice)


target_link_libraries( g PRIVATE  c++_static c   ${log-lib} ${OPENGL_LIB} android EGL m atomic z dl  $<BUILD_INTERFACE:juice>)
export(TARGETS g ANDROID_MK Android.mk)


add_executable(libnative.so    abbotttest.cpp )
target_include_directories( libnative.so PRIVATE  ${INCLUDESELF} ${INCLUDESELF}/share  LibAscon/inc)
target_link_libraries( libnative.so    ${log-lib} ${OPENGL_LIB} android EGL m g )
target_compile_definitions(libnative.so PUBLIC -DANDROID__APP )


add_library(native SHARED IMPORTED)

if(JUGGLUCO_APP)
else()
option(static "compile static" OFF)
add_executable(juggluco ${NETDIR}/watchserver/getlanguage.cpp ${CURVEDIR}/scriptFonts.cpp settings/rtl_label_converter.cpp ${NETDIR}/ICE/ContextHTTPS.cpp  ${NETDIR}/ICE/ICE.cpp  ${NETDIR}/ICE/ICE_data.cpp calibrate/calculate.cpp ${ASCONFILES} hostJson.cpp backupinfo.cpp export/libreviewexport.cpp  ${NETDIR}/watchserver/common.cpp  ${NETDIR}/watchserver/iob.cpp    ${NETDIR}/watchserver/watchserver.cpp  ${NETDIR}/watchserver/sslserver.cpp ${NETDIR}/watchserver/cryptopen.cpp   removeoldstates.cpp nums/numdata.cpp settings/settings.cpp  ${INCLUDESELF}/cmdline/qr.cpp  cmdline/main.cpp share/logs.cpp  ${NETDIR}/netstuff.cpp  ${NETDIR}/sender.cpp ${NETDIR}/sendcommands.cpp ${NETDIR}/backup.cpp ${NETDIR}/getcommand.cpp ${NETDIR}/myip.cpp  datbackup.cpp glucose.cpp ${CURVEDIR}/trjugglucotext.cpp ${ARJUGGLUCO_OUTPUT} ${CURVEDIR}/svjugglucotext.cpp    ${CURVEDIR}/zhjugglucotext.cpp  ${CURVEDIR}/ukjugglucotext.cpp  ${CURVEDIR}/bejugglucotext.cpp ${CURVEDIR}/frjugglucotext.cpp ${CURVEDIR}/pljugglucotext.cpp ${CURVEDIR}/ptjugglucotext.cpp ${CURVEDIR}/uzjugglucotext.cpp  ${CURVEDIR}/jajugglucotext.cpp ${CURVEDIR}/hijugglucotext.cpp ${CURVEDIR}/esjugglucotext.cpp ${CURVEDIR}/nljugglucotext.cpp ${CURVEDIR}/dejugglucotext.cpp  ${CURVEDIR}/jugglucotext.cpp  ${CURVEDIR}/itjugglucotext.cpp ${CURVEDIR}/rujugglucotext.cpp ${CURVEDIR}/curve.cpp ${CURVEDIR}/percentile.cpp  ${NANOVG}/nanovg.c export/exports.cpp settings/getcountry.cpp  meal/savemeals.cpp share/serial.cpp prunestream.cpp ${EXTRACMD}  )
target_sources(juggluco PRIVATE "${ARJUGGLUCO_OUTPUT}")
add_dependencies(juggluco generate_arjugglucotext)
target_include_directories( juggluco PRIVATE ${INCLUDESELF} ${INCLUDESELF}/cmdline ${INCLUDESELF}/share  ${INCLUDESELF}/LibAscon/inc ${NANOVG}  )
#target_compile_options(juggluco PUBLIC  -static  -pthread )
add_executable(readlog readlog.cpp)
target_include_directories( readlog PRIVATE  ${INCLUDESELF} ${INCLUDESELF}/share  )

if(HAVE_PTHREADARG)
target_compile_options(juggluco PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-pthread>)
endif()
if(static)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(juggluco PRIVATE
    m
    zstd
    z
    juice
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

if(OPENSSL_CRYPTO_LIBRARY MATCHES "\\.a$")
    execute_process(
        COMMAND ${CMAKE_NM} -u ${OPENSSL_CRYPTO_LIBRARY}
        OUTPUT_VARIABLE LIBCRYPTO_UNDEF
        ERROR_QUIET
    )

    if(LIBCRYPTO_UNDEF MATCHES "jent_")
        find_library(JITTERENTROPY_LIBRARY
            NAMES libjitterentropy.a
            REQUIRED
        )
        message(STATUS "OpenSSL static archive needs jitterentropy: ${JITTERENTROPY_LIBRARY}")
        target_link_libraries(juggluco PRIVATE ${JITTERENTROPY_LIBRARY})
    endif()
endif()


target_link_options(juggluco  PUBLIC -static )
target_compile_options(readlog PUBLIC  -fopenmp -static)
target_link_options(readlog  PUBLIC   -fopenmp -static  )
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES
    gcc_s_asneeded
    atomic_asneeded
)

list(REMOVE_ITEM CMAKE_CXX_IMPLICIT_LINK_LIBRARIES
    gcc_s_asneeded
    atomic_asneeded
)
set(extradefs -DUSE_SSL=1 )
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(juggluco PRIVATE Threads::Threads)
else()
message(" CMAKE_SYSTEM_NAME =${CMAKE_SYSTEM_NAME} ")

CHECK_INCLUDE_FILE_CXX(arm-linux-androideabi/asm/errno.h IsAndroid)
#if( IsAndroid || ${CMAKE_SYSTEM_NAME} MATCHES "Android")
if(IsAndroid)
target_link_libraries( juggluco     m  dl z  juice )
target_link_options(juggluco PUBLIC  -static-libstdc++)
message("is Android no pthread ")
set(extradefs -DUSE_SSL=1 -DANDROID_STL=c++_static )
else()
target_link_libraries( juggluco     m pthread dl z crypto ssl juice)
message(" pthread ")
set(extradefs -DUSE_SSL=1 )
endif()
target_compile_options(readlog PUBLIC  -fopenmp )
target_link_options(readlog  PUBLIC   -fopenmp  )

endif()

target_include_directories(juggluco PRIVATE ${INCLUDESELF} ${INCLUDESELF}/cmdline ${INCLUDESELF}/share ${INCLUDESELF}/include )
set(jugglucodef -DNOJAVA -DNOTAPP -DDONTSCALESTROKEWIDTH -DDONT_USE_LIBREVIEW=1 -DLIBRE3=1 -DDEXCOM=1 -DSIBIONICS=1 )

target_compile_definitions( juggluco  PRIVATE  ${jugglucodef} ${extradefs})
#-D_GLIBCXX_GTHREAD_USE_WEAK=0
get_target_property(sourjug juggluco SOURCE)
message(STATUS  ${sourjug})
set(NETPATH ${INCLUDESELF}/${NETDIR})
set(CURVEPATH ${INCLUDESELF}/${CURVEDIR})
set(depfiles ${RTLPP_SOURCE} ${NETPATH}/watchserver/getlanguage.cpp ${INCLUDESELF}/curve/scriptFonts.cpp ${INCLUDESELF}/settings/rtl_label_converter.cpp "${ARJUGGLUCO_OUTPUT}" "${ARJUGGLUCO_INPUT}" ${INCLUDESELF}/libjuice/src/addr.c ${INCLUDESELF}/libjuice/src/agent.c ${INCLUDESELF}/libjuice/src/base64.c ${INCLUDESELF}/libjuice/src/conn.c ${INCLUDESELF}/libjuice/src/conn_mux.c ${INCLUDESELF}/libjuice/src/conn_poll.c ${INCLUDESELF}/libjuice/src/conn_thread.c ${INCLUDESELF}/libjuice/src/const_time.c ${INCLUDESELF}/libjuice/src/crc32.c ${INCLUDESELF}/libjuice/src/hash.c ${INCLUDESELF}/libjuice/src/hmac.c ${INCLUDESELF}/libjuice/src/ice.c ${INCLUDESELF}/libjuice/src/juice.c ${INCLUDESELF}/libjuice/src/log.c ${INCLUDESELF}/libjuice/src/random.c ${INCLUDESELF}/libjuice/src/server.c ${INCLUDESELF}/libjuice/src/stun.c ${INCLUDESELF}/libjuice/src/tcp.c ${INCLUDESELF}/libjuice/src/timestamp.c ${INCLUDESELF}/libjuice/src/turn.c ${INCLUDESELF}/libjuice/src/udp.c ${NETPATH}/ICE/ContextHTTPS.cpp  ${NETPATH}/ICE/ICE.cpp  ${NETPATH}/ICE/ICE_data.cpp ${INCLUDESELF}/calibrate/calculate.cpp ${INCLUDESELF}/hostJson.cpp ${INCLUDESELF}/export/libreviewexport.cpp ${INCLUDESELF}/curve/trjugglucotext.cpp   ${INCLUDESELF}/curve/svjugglucotext.cpp    ${INCLUDESELF}/curve/zhjugglucotext.cpp  ${INCLUDESELF}/curve/ukjugglucotext.cpp  ${INCLUDESELF}/curve/bejugglucotext.cpp ${INCLUDESELF}/curve/frjugglucotext.cpp ${INCLUDESELF}/curve/pljugglucotext.cpp ${INCLUDESELF}/curve/ptjugglucotext.cpp ${INCLUDESELF}/curve/uzjugglucotext.cpp  ${INCLUDESELF}/curve/jajugglucotext.cpp  ${INCLUDESELF}/curve/hijugglucotext.cpp ${INCLUDESELF}/curve/esjugglucotext.cpp ${INCLUDESELF}/curve/nljugglucotext.cpp ${INCLUDESELF}/curve/dejugglucotext.cpp  ${INCLUDESELF}/curve/jugglucotext.cpp  ${INCLUDESELF}/curve/itjugglucotext.cpp ${INCLUDESELF}/curve/rujugglucotext.cpp ${INCLUDESELF}/curve/curve.cpp ${INCLUDESELF}/curve/percentile.cpp  ${INCLUDESELF}/curve/nanovg/src/nanovg.c ${INCLUDESELF}/backupinfo.cpp ${INCLUDESELF}/prunestream.cpp ${INCLUDESELF}/readlog.cpp ${ASCONFILES} ${INCLUDESELF}/removeoldstates.cpp  ${INCLUDESELF}/nums/numdata.cpp ${INCLUDESELF}/settings/settings.cpp  ${INCLUDESELF}/cmdline/qr.cpp   ${INCLUDESELF}/cmdline/main.cpp ${INCLUDESELF}/share/logs.cpp ${NETPATH}/watchserver/common.cpp  ${NETPATH}/watchserver/watchserver.cpp ${NETPATH}/watchserver/iob.cpp ${NETPATH}/watchserver/cryptopen.cpp    ${NETPATH}/watchserver/sslserver.cpp  ${NETPATH}/netstuff.cpp  ${NETPATH}/sender.cpp ${NETPATH}/sendcommands.cpp ${NETPATH}/backup.cpp ${NETPATH}/getcommand.cpp ${NETPATH}/myip.cpp ${INCLUDESELF}/datbackup.cpp ${INCLUDESELF}/glucose.cpp ${INCLUDESELF}/export/exports.cpp ${INCLUDESELF}/settings/getcountry.cpp  ${INCLUDESELF}/meal/savemeals.cpp ${INCLUDESELF}/share/serial.cpp  )
#add_custom_target(juggluco-dep g++ -MF juggluco-dep -MM -I${INCLUDESELF}/${NANOVG} -I${INCLUDESELF} -I${INCLUDESELF}/share  -I${INCLUDESELF}/LibAscon/inc ${depfiles} DEPENDS ${depfiles}) 
add_custom_target(juggluco-dep /usr/bin/echo -n doel:>juggluco-dep\;g++  ${jugglucodef} -MM -I${INCLUDESELF}/libjuice/include/juice -I${INCLUDESELF}/cmdline -I${INCLUDESELF}/curve -I${INCLUDESELF}/${NANOVG} -I${INCLUDESELF} -I${INCLUDESELF}/share  -I${INCLUDESELF}/LibAscon/inc ${depfiles}|sed "\'s/^.*\.o:.*$$//g;s/\\([^\\]\\)$$/&\\\\/g'" | head -c-2 | grep -v '^$$' >> juggluco-dep\;echo "doel:" >> juggluco-dep\; /usr/bin/echo -e \"\\ttar -chf cmdlinesrc.tar  \"  $$^ ${depfiles} >> juggluco-dep DEPENDS ${depfiles}) 
add_custom_target(cmdlinesrc.tar make -f ${CMAKE_CURRENT_BINARY_DIR}/juggluco-dep doel DEPENDS juggluco-dep)
endif()

