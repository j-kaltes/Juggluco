#pragma once
constexpr const int maxpenserial=40;
constexpr const int maxpennr=200;
struct NovoPen {
	uint64_t lasttime;
	int64_t type;
	char serial[maxpenserial];
	};

