cmake_minimum_required(VERSION 3.18)
project(rtlpp_host LANGUAGES CXX)

find_package(ICU REQUIRED COMPONENTS uc i18n)

add_executable(rtlpp
    "${CMAKE_CURRENT_LIST_DIR}/../../rtlpp.cpp"
)

target_link_libraries(rtlpp PRIVATE ICU::uc ICU::i18n)

