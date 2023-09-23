#pragma once
#include <cstdint>
struct LibreHistEl {
	uint32_t ti;
	uint16_t mgdL;
	uint16_t id;
	};
	

struct LibreHist {
	int64_t histor;
	LibreHistEl *list;
	int32_t size;
	uint32_t starttime;
	int msec;
	uint32_t notsend;
	uint32_t notsendHistory;
	bool sendstart;
	};
