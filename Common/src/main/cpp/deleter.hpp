#pragma once
struct deleter {
const char *ptr;
deleter(const char ptr[]): ptr(ptr){};
   void operator()(const char p[]) const {
   	if(p!=ptr)
		delete[] p;
    }
};
constexpr const char boolstr[][6]={"false","true"};
constexpr const char errormessage[]=R"(<h1>Error</h1>)";

