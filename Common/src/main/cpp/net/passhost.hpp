/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Fri Jan 27 12:38:18 CET 2023                                                 */


#pragma once
#include <array>
#include <string_view>
#include <sys/types.h>
#include <arpa/inet.h>
#if 0
        struct sockaddr_in6 {
               sa_family_t     sin6_family;   /* AF_INET6 */
               in_port_t       sin6_port;     /* port number */
               uint32_t        sin6_flowinfo; /* IPv6 flow information */
               struct in6_addr sin6_addr;     /* IPv6 address */
               uint32_t        sin6_scope_id; /* Scope ID (new in 2.4) */
           };

           struct in6_addr {
               unsigned char   s6_addr[16];   /* IPv6 address */
           }; 
#endif
#include "netstuff.hpp"
extern bool testhostname(const char *hostname,const struct sockaddr *addr);
extern bool sameaddress(const  struct sockaddr *addr, const struct sockaddr_in6  *known);

//union { struct sockaddr_in6 ips[passhost_t::maxip];
struct passhost_t {
inline static constexpr const int maxip=4;
// Zero deliberately means the legacy behaviour: existing databases used zero
// in these reserved bits.  Keeping that value as Automatic makes an upgraded
// phone/watch continue to use TCP with the established MessageClient fallback.
inline static constexpr uint8_t transport_automatic=0;
inline static constexpr uint8_t transport_tcp=1;
inline static constexpr uint8_t transport_messages=2;
inline static constexpr uint8_t transport_bluetooth=3;
struct hostnamedata {
    static constexpr const int maxhostname=sizeof(sockaddr_in6)*(passhost_t::maxip-1)-sizeof(uint16_t);
    uint16_t port; //host byte order
    char name[maxhostname];
};
inline static constexpr const int maxnamelen=16;
	struct sockaddr_in6 ips[maxip];
static_assert(sizeof(ips)==(sizeof(hostnamedata)+sizeof(sockaddr_in6)));
	int nr;
	int index;
	std::array<uint8_t,16>  pass;
	uint8_t receivefrom:5; 
	bool newconnection:1;
	bool hostname:1;
	bool wearos:1;
/*
Receive	   	reconnect	receivefrom:
true		true 	  	3
true	  	false	  	2
false		true 	 	1
false	  	false	  	0
*/

//	getupdatedata()->allhosts[index].receivefrom=receive?(reconnect?2:3):(sendto?1:0);

	uint8_t activereceive:4; // receiveactive=receive&&activeonly; Receive from named host active only
	bool detect:1;
	bool sendpassive:1;  // send to named host passive only
	bool hasname:1;
	bool noip:1;
	uint16_t reserved:8;
    // Existing rows have this previously-reserved bit clear and are therefore
    // treated as proven. New nearby mirror rows set it until one authenticated
    // BLE direction succeeds. A proven direction must never be reversed merely
    // because Android reports a transient GATT/advertising failure.
    bool bleunproven:1;
    // Pair-wide BLE direction preference for ordinary nearby mirrors. Both
    // peers store the SAME value; side then guarantees complementary roles:
    // client = (!side) XOR blereverse. This avoids two independently persisted
    // local roles drifting into client/client or server/server.
    bool blereverse:1;
	uint16_t mirrortransport:2;
	// Legacy/local physical BLE role used by Wear OS and older configuration
	// paths. Ordinary phone/tablet mirrors derive their role from side+blereverse.
	bool bleclient:1;
    // Permanent non-ICE pair identity: r1=false, r2=true. QR creation sends
    // the opposite value. For migrated rows r2 is the side that sends Scans.
    bool side:1;
    bool ICE:1;
	bool deactivated:1;
	uint8_t gettransport() const {
		return mirrortransport<=transport_bluetooth?mirrortransport:transport_automatic;
		}
	void settransport(int value) {
		mirrortransport=value>=transport_automatic&&value<=transport_bluetooth?value:transport_automatic;
		}
	bool automatictransport() const {
		return gettransport()==transport_automatic;
		}
	bool usesnetworktransport() const {
		const auto transport=gettransport();
		return transport==transport_automatic||transport==transport_tcp;
		}
	bool forcedmessagecarrier() const {
		const auto transport=gettransport();
		return transport==transport_messages||transport==transport_bluetooth;
		}
	bool hashostname() const {
		return hostname;
		}
	const char *gethostname() const {
		return reinterpret_cast<const hostnamedata*>(ips)->name;
		}
	char *gethostname() {
		return reinterpret_cast<hostnamedata*>(ips)->name;
		}
	void sethostname(std::string_view name) {
		int len=std::min(name.size(),sizeof(hostnamedata::name)-1);
		char *doel=reinterpret_cast<hostnamedata*>(ips)->name;
		memcpy(doel,name.data(),len);
		doel[len]='\0';
		}
	void setportwithhostname(int port)  {	
		 reinterpret_cast<hostnamedata*>(ips)->port=port;
		 }
    void setICEname(std::string_view name) {
		int len=std::min(name.size(),sizeof(hostnamedata::name)-1);
		char *doel=reinterpret_cast<hostnamedata*>(ips)->name;
		memcpy(doel,name.data(),len);
		doel[len]='\0';
		reinterpret_cast<hostnamedata*>(ips)->port=len;
        }
    std::string_view getICEname() const {
        return {reinterpret_cast<const hostnamedata*>(ips)->name, reinterpret_cast<const hostnamedata*>(ips)->port};
        }
	uint16_t getport() const {	
		if(hostname) {
			return reinterpret_cast<const hostnamedata*>(ips)->port;
			}
		return ntohs(ips[0].sin6_port);
		}
	bool isSender() const {
		return index>=0;
		}
        bool getActive() const {
            if(index>=0) {
                if(sendpassive) 
                    return false;
                if(activereceive)
                    return true;
                return !receivefrom;
                }
           return activereceive;
           }
bool passive() const {//Accept connections from this host
	return (receivefrom&&!activereceive)||sendpassive;
	}
bool getPassive() const {
        if(index>=0)
            return sendpassive;
        return receivefrom==2;
        }
bool receivedatafrom() const {
	return receivefrom&2;
	}
const char *getname() const {
	return reinterpret_cast<const char *>(ips+maxip-1);
	}
const char *getnameif() const {
	return hasname?getname():"%NONAME%";
	}
void setnameGen(const char *label,const void *padding)  {
	hasname=true;
	char *name=reinterpret_cast<char *>(ips+maxip-1);
	for(int i=0;i<maxnamelen;i++) {
		if(!label[i]) {
            memcpy(name+i,padding,maxnamelen-i);
			LOGGER("setname(%s)\n",name);
			return;
			}
		name[i]=label[i];
		}
	 LOGGER("setname(%s)\n",name);
	}
void setname(const char *label)  {
        setnameGen(label,zeros);
	}
        /*
void setnameX(const char *label)  {
        setnameGen(label,xes);
	} */
        
bool	haspass() const {
	const uint64_t *p=reinterpret_cast<const uint64_t *>(&pass);
	return *p||p[1];
	};
bool hasip(const struct sockaddr *addrptr) const {
        if(hashostname()) {
                return testhostname(gethostname(),addrptr);
                }
	for(int i=0;i<nr;i++)
		if(sameaddress(addrptr,ips+i))
			return true;
	return false;
	};
bool addiphasfamport(const struct sockaddr *addrptr) {
	if(nr>=maxip) {
		LOGGER("addiphasfamport %d>=maxip\n",nr);
		return false;
		}
	if(!::putiphasfamport( addrptr,ips+nr)) {
		LOGSTRING("addiphasfamport returns false\n");
		return false;
		}
	LOGSTRING("addiphasfamport returns true\n");
	++nr;
	detect=false;
	return true;
	};
bool putip(const sockaddr_in6  *addrptr) {
	*ips=*addrptr;
	if(!nr) nr++;
	detect=false;
	return true;
	}
void putips(const sockaddr_in6  *addrptr,int nrin) {
	const int usenr=std::max(0,std::min(nrin,maxip));
	if(usenr>0&&addrptr)
		memcpy(ips,addrptr,usenr*sizeof(ips[0]));
	nr=usenr;
	detect=false;
	}
bool putip(const struct sockaddr *addrptr) {
	if(!::putip( addrptr,ips)) {
		LOGSTRING("passhost_t::putip failed\n");
		return false;
		}
	if(!nr) nr++;
	detect=false;
	return true;
	};
};

#include <string_view>
extern void startreceiver(const char *port, passhost_t *hosts,int &hostlen) ;
extern void stopreceiver() ;

#include <string.h>
inline bool address(const passhost_t &host) {
	return !host.detect;
	}
