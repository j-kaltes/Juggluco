#pragma once 
#include <vector>
#include <string_view>
#include <span>
#include <openssl/ssl.h>

using namespace std::literals;
class ContextHTTPS {
private:
    SSL_CTX* ctx ;
    bool error=false;
static bool initLibrary();
public:

    static ContextHTTPS &getContext() ;
    ContextHTTPS();
    ~ContextHTTPS();
std::pair<std::vector<char>,int>  request(const std::string_view host,int port,const std::string_view path,const std::string_view TYPE,const std::span<const char> input) ;
std::pair<std::vector<char>,int>   getRequest(const std::string_view host,int port,const std::string_view path,const std::span<const char> input={})  {
    return  request(host, port,path,"GET"s, input) ;
    }
std::pair<std::vector<char>,int>  putRequest(const std::string_view host,int port,const std::string_view path,const std::span<const char> input={})  {
    return  request(host, port,path,"PUT"s, input) ;
    }
 };
