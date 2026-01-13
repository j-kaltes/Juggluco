#pragma once

template <typename IterType>
void getsensorranges(uint32_t start,uint32_t endt,bool calibrated,bool calibratePast,std::vector<GlucoseDataType<IterType>> *polldata);
extern template void getsensorranges<const ScanData*>(uint32_t start,uint32_t endt,bool calibrated,bool calibratePast,std::vector<GlucoseDataType<const ScanData*>> *polldata);
extern template void getsensorranges<HistoryIterator>(uint32_t start,uint32_t endt,bool calibrated,bool calibratePast,std::vector<GlucoseDataType<HistoryIterator>> *historydata ) ;
