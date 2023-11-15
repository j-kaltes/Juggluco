
#include <math.h>
#include "nums/numdata.h"
#include "nightnumcategories.h"
extern vector<Numdata*> numdatas;

/*
From "derived" from formula's in https://ph.pollub.pl/index.php/jcsi/article/view/1294
*/
//0.0000273563 (36554.6 - 5.8 t^2 + 0.0613333 t^3 - 0.0003045 t^4 + 8.146*10^-7 t^5 - 1.126*10^-9 t^6 + 6.29714*10^-13 t^7)
/*
long double iobfiasp(long double t){
	return 1. - 0.000158667 *powl(t,2) + 1.67786*powl(10,-6) *powl(t,3) - 8.33001*powl(10,-9) *powl(t,4) + 2.22845*powl(10,-11) *powl(t,5) - 3.08032*powl(10,-14) *powl(t,6) + 1.72267*powl(10,-17) *powl(t,7);
	}
long double iobfracold(long double t){return 0.0000273563*(36554.6 - 5.8 * powl( t,2) + 0.0613333 * powl(t,3) - 0.0003045* powl(t,4) + 8.146*powl(10,-7)*powl( t,5) - 1.126*powl(10,-9) * powl(t,6) + 6.29714*powl(10,-13)*powl( t,7));
    } */
//s/\([a-z0-9-.]*\)^\([0-9a-z-.]*\)/*powl(\1,\2)/g
//s/\*\*/*/g
static long double iobNovoRapid(long double t) {
	return 1. - 0.00013146 *powl(t,2) + 1.32911*powl(10,-6) *powl(t,3) - 6.55082*powl(10,-9) *powl(t,4) + 1.77615*powl(10,-11) *powl(t,5) - 2.50625*powl(10,-14) *powl(t,6) + 1.43098*powl(10,-17) *powl(t,7);
 	}

static long double iobformula(long double level,long double age) {
	return iobNovoRapid(age)*level;
	}
static long double getoneiob(long double value,long double weight,long double age) {
	long double level=value*weight;
	return iobformula(level,age);
	}
double getiob(uint32_t now) {
	uint32_t oldage=now-6*60*60;
	long double iob=0.0L;
	for(const auto *nd:numdatas) {
		const Num *start=nd->begin();
		const Num *last=nd->end()-1;
		for(const Num *it=last;it>=start;it--) {
			double weight;
			if(nd->valid(it))  {
				if(it->time<=oldage)
					break;
				if((weight=rapidNightWeight(it->type))!=0.0f) {
					long double agemin= (now-it->time)/60;
					iob+=getoneiob(it->value,weight,agemin);
					}
				}
			}
		}
	return iob;
	}
