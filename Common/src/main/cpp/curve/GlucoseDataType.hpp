#pragma once
#include <type_traits>
#include <utility>
#include "calibrate/Calibrate.hpp"
template <typename Type1,typename Type2=Type1>
struct GlucoseDataType  {
    Type1 first;
    Type2 second;
    const int idDistance;
    bool calibrated;
    CalibrateForward<std::remove_cvref_t<decltype(*std::declval<Type1>())>> cali;
    };
