# Include after add_library(g ...) in Juggluco's native CMakeLists.txt.
if(ANDROID AND NOT WEAROS)
    set(JGCHAT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
    target_sources(g PRIVATE
        "${JGCHAT_ROOT}/src/auth.cpp"
        "${JGCHAT_ROOT}/src/client.cpp"
        "${JGCHAT_ROOT}/src/tools.cpp"
        "${JGCHAT_ROOT}/src/plot.cpp"
        "${JGCHAT_ROOT}/src/files.cpp"
        "${JGCHAT_ROOT}/src/chat_export.cpp"
        "${JGCHAT_ROOT}/src/storage.cpp"
        "${JGCHAT_ROOT}/src/workspace.cpp"
        "${JGCHAT_ROOT}/src/query.cpp"
        "${JGCHAT_ROOT}/src/numerics.cpp"
        "${JGCHAT_ROOT}/src/analysis_sqlite.c"
        "${JGCHAT_ROOT}/src/http_response.cpp"
        "${JGCHAT_ROOT}/src/native_https.cpp"
        "${JGCHAT_ROOT}/android/juggluco_data.cpp"
        "${JGCHAT_ROOT}/android/juggluco_nutrition.cpp"
        "${JGCHAT_ROOT}/android/juggluco_activities.cpp"
        "${JGCHAT_ROOT}/android/juggluco_statistics.cpp"
        "${JGCHAT_ROOT}/android/juggluco_system.cpp"
        "${JGCHAT_ROOT}/android/juggluco_system_native.cpp"
        "${JGCHAT_ROOT}/android/juggluco_configuration.cpp"
        "${JGCHAT_ROOT}/src/timeline.cpp"
        "${JGCHAT_ROOT}/android/jni_bridge.cpp")
    target_include_directories(g PRIVATE "${JGCHAT_ROOT}/include"
        "${JGCHAT_ROOT}/android" "${JGCHAT_ROOT}/vendor")
    # Keep Juggluco's current C++ dialect; this module requires C++20 or later.
    # Existing native OpenSSL/BoringSSL loader, libc, pthread and dl are reused.
    # Neither libcurl nor a Codex executable/Rust runtime is required on Android.
endif()
