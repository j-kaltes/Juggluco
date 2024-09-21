#pragma once
       #include <unistd.h>
       #include <sys/syscall.h>
              #include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <algorithm>
struct slogSetbase {
	int base;
	};
inline slogSetbase  slbase(int b) {
	return {b};
	};
#ifdef NOLOG

class slprint{
	public:
	template <class T, std::size_t N> slprint& operator<<(const T (&array)[N]) noexcept
	{
	  return *this;	
	}

	 slprint& operator<<(slogSetbase b) noexcept {
		return *this;
	}


	template <class T, std::size_t N> slprint& operator<<(T (&array)[N]) noexcept
	{
	  return *this;	
	}

	template <typename T, std::enable_if_t<std::is_convertible<T, char const*>::value , bool> = true > slprint & operator<<(const T s) {
		return *this;	
		}

	template <typename T, std::enable_if_t<std::is_arithmetic<T>::value , bool> = true > slprint& operator<<( const T value )  {
		return *this;
		}
 slprint& operator<<( slprint &val )  {
        return *this;
        }

typedef slprint& (*proc_t)(slprint&);

 slprint& operator<<(proc_t arg )  {
        return *this;
        }


  ~slprint() {
        }

};
class slog: public slprint {
public:
    slog() {
        }

};

extern  slprint& endl(slprint& os);
inline   slprint& endl(slprint & os) { return os; }
#else
#ifdef TEST
void logwriter(const char *buf,int len) {
	write(STDOUT_FILENO,buf,len);
	}
#else
#include "logs.hpp"
#endif
#include <charconv>

class slprint{
	static constexpr const int maxbuf=4096;
	char buf[maxbuf];
	char *bufptr=buf;
	char *endbuf=buf+maxbuf;
	int base=10;
	public:


	void addbuf(const char *array,int len) {
		int takelen=std::min(len,(int)(endbuf-bufptr));
	       memcpy(bufptr,array,takelen);
	       bufptr+=takelen;
		}
	template <class T, std::size_t N> slprint& operator<<(const T (&array)[N]) noexcept
	{
	    addbuf(array, N-1);
	  return *this;	
	}
	 slprint& operator<<(const bool value) noexcept {
	  	if(value) return operator<<("true");
		return operator<<("false");
	}

	 slprint& operator<<(slogSetbase b) noexcept {
	 	base=b.base;
		return *this;
	}


	template <class T, std::size_t N> slprint& operator<<(T (&array)[N]) noexcept
	{
	    addbuf(array,strlen(array));
	  return *this;	
	}

	//template <typename T, std::enable_if_t<std::is_pointer<T>::value , bool> = true > 
	template <typename T, std::enable_if_t<std::is_convertible<T, char const*>::value , bool> = true > slprint & operator<<(const T s) {
	       addbuf(s,strlen(s));
		return *this;	
		}


	template <typename T, std::enable_if_t<std::is_arithmetic<T>::value , bool> = true > slprint& operator<<( const T value )  {
		std::to_chars_result res=std::to_chars(bufptr,endbuf,value,base);
		bufptr=res.ptr;
		return *this;
		}
 slprint& operator<<( slprint &val )  {
        return *this;
        }
typedef slprint& (*proc_t)(slprint&);
 slprint& operator<<(proc_t arg )  {
	extern  slprint& endl(slprint& os);
	if(arg==endl) {
		*bufptr++='\n';
		}	 
        return *this;
        }


  ~slprint() {
	logwriter(buf,bufptr-buf);
        }

};
class slog: public slprint {
public:
    slog() {
	*this<<time(nullptr)<<" "<<syscall(SYS_gettid)<<" ";
        }

};

extern  slprint& endl(slprint& os);
inline   slprint& endl(slprint & os) { return os; }
#endif
#ifdef MAIN
int main() {
slog log;
log<<"hallo this me "<<5<<" "<<6<<endl;
}
#endif
