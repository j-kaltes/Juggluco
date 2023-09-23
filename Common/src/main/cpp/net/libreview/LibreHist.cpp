#include  "LibreHist.hpp"
struct Glucose { 
uint32_t time;
uint16_t id;
uint16_t glu[];
uint16_t getraw() const { return glu[0];};
uint16_t getsputnik() const { return glu[1];};
uint16_t getmgdL() const { return glu[1]/10;};
uint32_t gettime() const {return time;};
uint32_t getid() const {return id;};
bool isStreamed() const {
	return glu[2]&0x4000;
	}
void setStreamed()  {
	glu[2]|=0x4000;
	}
bool isLibreSend() const {
	return glu[2]&0x8000;
	}
void setLibreSend()  {
	glu[2]|=0x8000;
	}
void unSetLibreSend() {
	glu[2]&=~0x8000;
	}
bool valid() const {
	return glu[2]&0x4000;
	}
};
int main() {
Glucose glar[5];
glar[0]={.time=100,.id=145};
const Glucose *gl=glar;
struct LibreHistEl  *list=new LibreHistEl[50];
// list[0]={.ti=gl->gettime(),.mgdL=gl->getmgdL(),.id=(uint16_t)gl->getid()};
 //               const Glucose *gl=sens->getglucose(iter);
 int uitit=0;
                if(gl->valid()&&!gl->isLibreSend()) {
			 list[uitit++]={.ti=gl->gettime(),.mgdL=gl->getmgdL(),.id=(uint16_t)gl->getid()};
                         list[uitit++]={.ti=gl->gettime(),.mgdl=gl->getmgdL(),.id=(uint16_t)gl->getid()};
                        }

	}
