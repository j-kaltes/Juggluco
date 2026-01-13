#pragma once
  static constexpr const int glucosetype=0x40000000;//std::numeric_limits<int>::max();
  static constexpr const int nosearchtype=0x20000000;//std::numeric_limits<int>::max();
    static constexpr const int historysearchtype=2|glucosetype;
    static constexpr const int scansearchtype=1|glucosetype;
    static constexpr const int streamsearchtype=4|glucosetype;
    static constexpr const int calibratedStreamsearchtype=8|glucosetype;
    static constexpr const int calibratedHistorysearchtype=16|glucosetype;
