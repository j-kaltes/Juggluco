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
/*      Fri Nov 22 12:20:22 CET 2024                                                 */


#ifdef DEXCOM
#include <dlfcn.h>

#include <stdio.h>
#include <array>
#include <stdint.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <string_view>
#include <algorithm>
#include <span>
#include "ecJPake.hpp"

#include "cryptodecl.hpp"
extern void* opencrypto();
#ifdef TEST
void* opencrypto() {
#ifdef __ANDROID_API__
	#if defined(__aarch64__) || defined(__x86_64__) 
	const char *lib="/system/lib64/libcrypto.so";
	#else
	const char *lib="/system/lib/libcrypto.so";
	#endif
#else
	const char *lib="/usr/lib/libcrypto.so";
	#endif
  return dlopen(lib,RTLD_NOW);
  }

#endif
static bool doinitcryptofuncs() {
   LOGAR("doinitcryptofuncs");
   #define hgetsym(handle,name) *((void **)&name##ptr)=dlsym(handle, #name)
   #define getsym(name) hgetsym(handle,name)
   #define symtest(name) if(!name##ptr&&!(getsym(name))) { dlclose(handle);LOGGER(#name ": %s\n",dlerror());return false;}
   void *handle=opencrypto();
   if(!handle)  {
      return false;
      }
   symtest(BN_CTX_new)
   symtest(BN_add)
   symtest(BN_bin2bn)
   symtest(BN_bn2bin)
   //symtest(BN_bn2binpad)
   symtest(BN_bn2hex)
   symtest(BN_div)
   symtest(BN_dup)
   symtest(BN_free)
   symtest(BN_hex2bn)
   symtest(BN_mod_mul)
   symtest(BN_mul)
   /*
   if(!(getsym(BN_native2bn))) {
     if(!( *((void **)& BN_native2bnptr)=dlsym(handle, "BN_le2bn"))) {
	 LOGGER("BN_le2bn: %s\n",dlerror());
	 return false;
	 }
      }*/
   symtest(BN_new)
   symtest(BN_nnmod)
   symtest(BN_rand_range)
   symtest(BN_sub)
   symtest(EC_POINT_add)
   symtest(EC_POINT_cmp)
   symtest(EC_POINT_dup)
   symtest(EC_POINT_free)

   if(!(getsym(EC_POINT_get_affine_coordinates))) {
     if(!( *((void **)& EC_POINT_get_affine_coordinatesptr)=dlsym(handle, "EC_POINT_get_affine_coordinates_GFp"))) {
	 LOGGER("EC_POINT_get_affine_coordinates: %s\n",dlerror());
	 return false;
	 }
      }

   symtest(EC_POINT_mul)
   symtest(EC_POINT_new)
   symtest(EC_POINT_oct2point)
   symtest(EC_POINT_point2oct)

   if(!(getsym(EC_POINT_set_affine_coordinates))) {
     if(!( *((void **)& EC_POINT_set_affine_coordinatesptr)=dlsym(handle, "EC_POINT_set_affine_coordinates_GFp"))) {
	 LOGGER("EC_POINT_set_affine_coordinates: %s\n",dlerror());
	 return false;
	 }
      }

   symtest(EVP_DigestFinal)
   symtest(EVP_DigestInit)
   symtest(EVP_DigestUpdate)

   if(!(getsym(EVP_MD_CTX_free))) {
     if(!( *((void **)& EVP_MD_CTX_freeptr)=dlsym(handle, "EVP_MD_CTX_destroy"))) {
	 LOGGER("BN_le2bn: %s\n",dlerror());
	 return false;
	 }
      }
   if(!(getsym(EVP_MD_CTX_new))) {
     if(!( *((void **)& EVP_MD_CTX_newptr)=dlsym(handle, "EVP_MD_CTX_create"))) {
	 LOGGER("BN_le2bn: %s\n",dlerror());
	 return false;
	 }
      }
    symtest(EVP_sha256);
      symtest(ERR_clear_error);
   //symtest(CRYPTO_free);
   if(!(getsym(OPENSSL_free))) {
	   OPENSSL_freeptr=free;
      }
   // symtest(EC_GROUP_get0_order);
    symtest(EC_GROUP_get_order);
   symtest(ERR_print_errors_cb);
   symtest(EC_GROUP_get0_generator);
   //symtest(EC_GROUP_get_curve);

   if(!(getsym(EC_GROUP_get_curve))) {
     if(!( *((void **)&EC_GROUP_get_curveptr)=dlsym(handle, "EC_GROUP_get_curve_GFp"))) {
	 LOGGER("EC_GROUP_get_curve: %s\n",dlerror());
	 return false;
	 }
      }
   symtest(EC_GROUP_get_cofactor);
   symtest(EC_KEY_get0_private_key);
   symtest(EC_KEY_get0_public_key);
   symtest(EC_KEY_free);
   symtest(EC_KEY_new_by_curve_name);
   symtest(EC_KEY_generate_key);
   symtest(EC_GROUP_new_by_curve_name)
   symtest(EC_KEY_generate_key);
   symtest(EC_KEY_set_private_key)
   symtest(EC_KEY_set_public_key)
   symtest(ECDSA_size)
   symtest(ECDSA_sign)
   symtest(AES_set_encrypt_key)
   symtest(AES_encrypt)

   return true;
   }

static bool hasCrypto() {
  static bool canCrypt=doinitcryptofuncs();
  return canCrypt;
  }

#include "cryptodef.h"
#include "types.hpp"
#ifdef TEST
#define LOGGER(...) fprintf(stderr,__VA_ARGS__)
#define LOGAR(...) fprintf(stderr,"%s\n",__VA_ARGS__)
#define LOGGERN(buf,len) fwrite(buf,len,1,stderr);
#endif
typedef uint8_t uchar;
static int showhex(std::string_view name,const uchar *data, const int datalen) {
#ifndef NOLOG
   char buf[1024],*ptr=buf,*end=std::end(buf)-1;;
   memcpy(ptr,name.data(),name.size()) ;
   ptr+=name.size();
   for(int i=0;i<datalen;++i) {
	 ptr+=snprintf(ptr,end-ptr,"%02x",data[i]);
	  }
   int len=ptr-buf;
   LOGGERN(buf,len);
   return len;
#else
   return 0;
#endif
   }
template <class T> int showhex(std::string_view name,T inp) {
	return showhex(name,inp.data(),inp.size());
	}
//constexpr const size_t SHA256_DIGEST_LENGTH = 256 / 8;
#define THREAD_LOCAL thread_local 
static BN_CTX *getBN_CTX() {
   static THREAD_LOCAL BN_CTX *bn_ctx= BN_CTX_new();
   return bn_ctx;
   }

static BIGNUM* mkorder() {
   BIGNUM *order=BN_new();
   EC_GROUP_get_order(getgroup(), order, getBN_CTX());
   return order;
   }

static const BIGNUM * getorder() {
   static const BIGNUM* order=mkorder();
   return order;
	}
static std::array<uchar,SHA256_DIGEST_LENGTH> sha256(const uchar *input,const int len){
   EVP_MD_CTX* ctx = EVP_MD_CTX_new();
   EVP_DigestInit(ctx,EVP_sha256()); 
   EVP_DigestUpdate(ctx, input, len);
   std::array<uchar,SHA256_DIGEST_LENGTH>  digest;
   EVP_DigestFinal(ctx, digest.data(), NULL);
   EVP_MD_CTX_free(ctx);
   return digest;
   }



static int  tobytes(const EC_POINT * P,uchar *data) {
   Number x,y;
   EC_POINT_get_affine_coordinates(getgroup(),P,x,y, getBN_CTX());
   int len1=BN_bn2bin(x,data);
   int len2=BN_bn2bin(y,data+len1);
   return (len1+len2);
	}
static EC_POINT *frombytes(const uchar *data) {
   Number x(BN_bin2bn(data,32,nullptr));
   Number y(BN_bin2bn(data+32,32,nullptr));
   auto P=EC_POINT_new(getgroup());
   EC_POINT_set_affine_coordinates(getgroup(),P,x,y, getBN_CTX());
   return P;
	}



static int  tobytes4(const EC_POINT * P,uchar *data) {
   return EC_POINT_point2oct(getgroup(), P, POINT_CONVERSION_UNCOMPRESSED,data,65, getBN_CTX());
	}
    /*
static EC_POINT  *frombytes4(uchar *data) {
   auto group=getgroup();
  EC_POINT *P=EC_POINT_new(group);
   if(EC_POINT_oct2point(group, P,data,65, getBN_CTX()))
      return P;
  return nullptr;
	}
    */
#include <assert.h>

static void setnum(uchar *buf,int len) {
   uchar *lenptr=reinterpret_cast<uchar *>(&len);
   for(int i=0;i<4;i++) {
      buf[i]=lenptr[3-i];
      }
   }

#include <memory>
template <typename T>
struct  SSLdeleter {
   void operator()(T p) {
        OPENSSL_freeptr((void *)p);
       };
  };
template <typename T=const char[]>
   using SSLfree=std::unique_ptr<T,SSLdeleter<const std::remove_extent_t<T> *>>;

void printNum(const char *name,const BIGNUM *bigint) {
   LOGGER("%s %s\n",name,SSLfree<>(BN_bn2hex(bigint)).get());
   }

void printPoint(std::string_view name,const EC_POINT* p) {
#ifndef NOLOG
   constexpr const int pubbufmax=512;
   unsigned char pubbuf[pubbufmax];
   auto publen=EC_POINT_point2oct(getgroup(), p, POINT_CONVERSION_UNCOMPRESSED,pubbuf,pubbufmax,getBN_CTX());
   char buf[pubbufmax],*ptr;
   memcpy(buf,name.data(),name.size());
   ptr=buf+name.size();
   for(int i=0;i<publen;i++) {
      ptr+=sprintf(ptr,"%02x",pubbuf[i]);
      }
 //  memcpy(ptr,"\n",2);
  // ptr+=1;
   LOGGERN(buf,ptr-buf);
#endif
   }

  /* 
void ec_point_print_fp( FILE *fp,const EC_POINT* p) {
   printPoint("point: ",p);
   }
void ec_point_print_fp( FILE *fp,const EC_POINT* p) {
   constexpr const int pubbufmax=512;
   unsigned char pubbuf[pubbufmax];
   auto publen=EC_POINT_point2oct(getgroup(), p, POINT_CONVERSION_UNCOMPRESSED,pubbuf,pubbufmax,getBN_CTX());
   char buf[pubbufmax],*ptr=buf;
   for(int i=0;i<publen;i++) {
      ptr+=sprintf(ptr,"%02X",pubbuf[i]);
      }
   memcpy(ptr,"\n",2);
   fwrite(buf,ptr-buf+1,1,fp);
*/
static auto mkhash(const EC_POINT *p1,const EC_POINT * gv, const EC_POINT * pub_key, const std::array<const uchar,6> party) {
   const int ec_size=65;
   const int buflen= 4*4+party.size()+3*ec_size;
   uchar alldata[buflen];
   uchar *dataptr=alldata;
   setnum(dataptr,ec_size);
   dataptr+=4;
   tobytes4(p1, dataptr);
   dataptr+=ec_size;
   setnum(dataptr,ec_size);
   dataptr+=4;
   tobytes4(gv, dataptr);
   dataptr+=ec_size;
   setnum(dataptr,ec_size);
   dataptr+=4;
   tobytes4(pub_key, dataptr);
   dataptr+=ec_size;
   setnum(dataptr,party.size());
   dataptr+=4;
   memcpy(dataptr,party.data(),party.size());
   dataptr+=party.size();
   showhex("to hash: ",alldata,buflen);
   return sha256(alldata,buflen);
   }

constexpr const std::array<const uchar,6> a_bytes{0x63,0x6c,0x69,0x65,0x6e,0x74};
constexpr const std::array<const uchar,6> b_bytes{0x37,0x56,0x27,0x67,0x56,0x27};
static  BIGNUM* mkhashBigInt(const EC_POINT *p1,const EC_POINT * pub_key,const EC_POINT * gv,  const BIGNUM*order,  const std::array<const uchar,6> bytes) {
   const auto hash=mkhash(p1,gv, pub_key,bytes);
   showhex("digest: ",hash);
	BIGNUM*  bigint=BN_bin2bn(hash.data(),hash.size(),nullptr);
   printNum("bigint=",bigint);
   BN_nnmod(bigint, bigint, order,getBN_CTX());
   printNum("mkhashBigInt=",bigint);
   return bigint;
   }

static BIGNUM* getproof(const EC_POINT *p1,const EC_POINT*pub_key,const EC_POINT *gv,const BIGNUM *  private_key,const BIGNUM * rannum) {
   auto bnctx=getBN_CTX();

   const BIGNUM* order= getorder();
   printNum("order",order);
   BIGNUM* bigint= mkhashBigInt(p1, pub_key, gv,order,a_bytes);
   BN_mul(bigint,bigint,private_key,bnctx);
   printNum("bigint*priv_key",bigint);
   BN_sub(bigint, rannum, bigint);
   printNum("rannum-bigint",bigint);
  BN_nnmod(bigint, bigint, order, bnctx);

//   BN_div(nullptr,bigint, bigint, order, bnctx);
   printNum("proof=",bigint);
   return bigint; 
   }
void PCert::frombytes(const uchar *bytes){
   pubkey1=::frombytes(bytes);
   pubkey2=::frombytes(bytes+64);
   hash=BN_bin2bn(bytes+128,32,nullptr);
   }
void PCert::free() {
    
   this->~PCert();
//   memset(this,'\0',sizeof(*this));;
   bzero(this,sizeof(*this));
   }
PCert::~PCert() {
      LOGGER("start ~PCert() this=%p\n",this);
      EC_POINT_free(const_cast<EC_POINT*>(pubkey1));
      EC_POINT_free(const_cast<EC_POINT*>(pubkey2));
      BN_free(const_cast<BIGNUM*>(hash));
      LOGGER("end ~PCert() this=%p\n",this);
      };
PCert::PCert(const PCert &cert): pubkey1(EC_POINT_dup(cert.pubkey1,getgroup())), pubkey2(EC_POINT_dup(cert.pubkey2,getgroup())), hash(BN_dup(cert.hash)) {
      LOGGER("PCert(&) this=%p\n",this);
      }
/*
void PCert::fill(const EC_POINT * p1,const EC_POINT * pub_key,const BIGNUM *  private_key,const BIGNUM * rannum){
   LOGGER("PCert::fill const EC_POINT * this=%p\n",this);
   fill( p1,std::move(EC_POINT_dup(pub_key,getgroup())),private_key,rannum);
   }
void PCert::fill(const EC_POINT * p1,Point &&pub_key,const BIGNUM *  private_key,const BIGNUM * rannum){
   LOGGER("PCert::fill Point && this=%p\n",this);
   fill( p1,std::move(pub_key.data),private_key,rannum);
   pub_key.data=nullptr;
   }
void PCert::fill(const EC_POINT * p1,const Point &pub_key,const BIGNUM *  private_key,const BIGNUM * rannum){
   LOGGER("PCert::fill const Point & this=%p\n",this);
   fill( p1,std::move(EC_POINT_dup(pub_key.data,getgroup())),private_key,rannum);
   }
*/
void PCert::fill(const EC_POINT * p1, const EC_POINT * pub_key,const BIGNUM *  private_key,const BIGNUM * rannum){
   auto group=getgroup();
   pubkey1=EC_POINT_dup(pub_key,group);
   LOGGER("PCert::fill this=%p\n",this);
   auto bnctx=getBN_CTX();
   EC_POINT  *gv=EC_POINT_new(group);
   EC_POINT_mul(group,gv,nullptr,p1,rannum,bnctx);
   printPoint("gv=",gv);
   pubkey2=gv; 
   hash=getproof(p1,pub_key,gv,private_key,rannum);
//   return  {pub_key,gv,bigint}; 
   }

std::array<uchar,160> PCert::byteify() const {
   std::array<uchar,160> packet;
   auto *data=packet.data();
   int len=tobytes(pubkey1,data);
   LOGGER("pubkey1 len=%d\n",len);
   int len2=tobytes(pubkey2,data+len);
   LOGGER("pubkey2 len=%d\n",len2);
   len+=len2;
#ifndef NOLOG
   size_t lenuit=
#endif
   BN_bn2bin(hash,data+len);

   LOGGER("byteify len=%d lenuit=%zd\n",len,lenuit);

//   assert((lenuit+len)==160);
   return packet;
}

static auto mkround12(const EC_POINT * p1,const EC_POINT * pub_key,const BIGNUM *  private_key,const BIGNUM * rannum) {
   LOGAR("mkround12");
   PCert cert;
   cert.fill(p1, pub_key, private_key,rannum);
   return cert.byteify();
   }
int logcallback(const char *str, size_t len, void *u);

#ifdef TEST
int logcallback(const char *str, size_t len, void *u) {
    constexpr const int maxbuf=1024;
    char buf[maxbuf];
   const char *format=(const char *)u;
   auto nr=snprintf(buf,maxbuf,format,str);
   LOGGERN(buf,nr);
   return 0;
   }
#endif

static void  showerror(const char *format) {
   ERR_print_errors_cb(logcallback,(void*)format);
   }

bool makeRound3Cert(PCert &cert,const EC_POINT * pub1,const EC_POINT * pub2,const EC_POINT *  pubA,const BIGNUM * privB,const BIGNUM * pass) {
   ERR_clear_error();
   const auto ec_group=getgroup();
   auto bn_ctx=getBN_CTX();
   Number x2s;
   Point g134,A;
   if(!BN_mod_mul(x2s,privB,pass,getorder(),bn_ctx)) {
      showerror("BN_mod_mul failed %s");
      ERR_clear_error();
      return  false;
   	}
  printPoint("x1=",pubA);
  printNum("x2=",privB);
  printPoint("x3=",pub1);
  printPoint("x4=",pub2);
  printNum("s=",pass);
  printNum("x2s=",x2s);
   if(!EC_POINT_add(ec_group, g134,pubA,pub1, bn_ctx)) {
      showerror("EC_POINT_add(ec_group, g134,pubA,pub1 failed %s");
      ERR_clear_error();
      return  false;
   	}
   if(!EC_POINT_add(ec_group, g134,g134,pub2, bn_ctx)){
      showerror("EC_POINT_add(ec_group, g134,g134,pub2 failed %s");
      ERR_clear_error();
      return false;
   	}
  printPoint("g134=",g134);
  if(!EC_POINT_mul(ec_group,A,nullptr,g134,x2s,bn_ctx)) {
      showerror("EC_POINT_mul(ec_group,A,nullptr,g134,x2s,bn_ctx) %s");
      ERR_clear_error();
      return false;
   	}
  printPoint("A=",A);


   Number ran3;
   BN_hex2bn(&ran3.data,"fbc971b837e9491e45a4179ed33865c508a1e0a1d350f5af0f96370695fdc393");
   printNum("Exponent",ran3);


   LOGAR("end makeRound3Cert");
   cert.fill(g134,std::move(A),  x2s, ran3);
   return true;
   }
typedef uint8_t byte;
static const BIGNUM *getone() {
   constexpr const uchar onebuf[]{0,0,0,1};
   static const BIGNUM* one=BN_bin2bn(onebuf,sizeof(onebuf),nullptr);
   return one;
   }
static const BIGNUM *gettwo() {
   constexpr const uchar twobuf[]{0,0,0,1};
   static const BIGNUM* two=BN_bin2bn(twobuf,sizeof(twobuf),nullptr);
   return two;
   }

static BIGNUM *mkminone() {
   BIGNUM* minone=BN_new();
   BN_sub(minone,minone,getone());
   return minone;
   }
static  BIGNUM *getminone() {
   BIGNUM* minone=mkminone();
   return minone;
   }

static BIGNUM *getrandom() {
   BIGNUM *ran=BN_new();
   BN_sub(ran,getorder(),gettwo());
   BN_rand_range(ran, ran);
   BN_add(ran,ran,getone());
   return ran;
   }
auto mkSharedKey(const PCert &cert2,const PCert &cert3,const BIGNUM *pass,const BIGNUM *x2) {
   auto bn_ctx=getBN_CTX();
   const auto ec_group=getgroup();
   const auto point1 = cert3.pubkey1;
   const auto g4 = cert2.pubkey1;
   Number num;
   BN_mod_mul(num,x2,pass,getorder(),bn_ctx);
   Point key;
   Number minone(getminone()); 
   BN_mul(num,num,minone,bn_ctx);
   EC_POINT_mul(ec_group, key, nullptr, g4, num, bn_ctx);
   EC_POINT_add(ec_group, key,point1,key, bn_ctx);
   EC_POINT_mul(ec_group, key, nullptr, key, x2, bn_ctx);
   Number x;
  EC_POINT_get_affine_coordinates(ec_group,key, x, nullptr, bn_ctx);
  constexpr const int buflen=32;
  uchar buf[buflen]{};

   BN_bn2bin(x, buf);
   auto sha=sha256(buf,buflen);
   showhex("mkSharedKey: ",sha.data(),sha.size());
   return sha;
    }
// https://www.rfc-editor.org/rfc/rfc8235?trk=public_post_comment-text#page-7
// g*r+A*c==gv
static bool validateZeroKnowledgeProof(const EC_POINT *g, const PCert &cert, const std::array<const uchar,6> party ) {
   const auto A= cert.pubkey1;
   const auto gv=cert.pubkey2;
   printPoint("validateZeroKnowledgeProof A=",A);
   const auto r=cert.hash;
   const auto group=getgroup();
   BN_CTX *bn_ctx=getBN_CTX();
   const Number c(mkhashBigInt(g,A,gv,getorder(),party));
   Point res1,res2;

   EC_POINT_mul(group, res1, nullptr, g, r, bn_ctx);
   EC_POINT_mul(group, res2, nullptr, A, c, bn_ctx);
   EC_POINT_add(group, res1,res1,res2, bn_ctx);
   return EC_POINT_cmp(group, res1, gv, bn_ctx)==1;
    }
bool validate12(const PCert &cert) {
   const auto g=EC_GROUP_get0_generator(getgroup());
   return validateZeroKnowledgeProof(g, cert,b_bytes);
   }



bool validate3(const EC_POINT *g1,const EC_POINT *g2,const PCert &cert1,const PCert &cert3) {
   BN_CTX *bn_ctx=getBN_CTX();
     auto g3 = cert1.pubkey1;
     Point g;
     const auto group=getgroup();
     EC_POINT_add(group, g,g1,g2, bn_ctx);
     EC_POINT_add(group, g,g,g3, bn_ctx);
     printPoint("validate3 g3=",g3);
     bool res= validateZeroKnowledgeProof(g, cert3, b_bytes);
     LOGGER("validate3()=%d\n",res  );
     return res;
    }



const BIGNUM *KeyPair::getprivate() const {
     return EC_KEY_get0_private_key(keyptr);
     }
const EC_POINT *KeyPair::getpublic() const {
     return EC_KEY_get0_public_key(keyptr);
     }
KeyPair::~KeyPair() {
  LOGGER("%p->~KeyPair()\n",this);
  EC_KEY_free(keyptr);
  keyptr=nullptr;
  }
KeyPair::KeyPair() {
  LOGGER("%p->KeyPair()\n",this);
  keyptr = EC_KEY_new_by_curve_name(curveNID);; //Als BIGNUM en EC_POINT apart opgeslagen worden moeten ze gekopieerd worden
  EC_KEY_generate_key(keyptr); 
  }

std::array<uchar,160>  mkround12(const KeyPair &keys) {
	return mkround12(EC_GROUP_get0_generator(getgroup()), keys.getpublic(),keys.getprivate(),getrandom());
	}
//bool initfuncs();

static EC_KEY *mkkeyC() {
      auto keys = EC_KEY_new_by_curve_name(curveNID);;
constexpr const uint8_t privbuf[]={0x7c,0xfb,0xd5,0x96,0xf6,0xe7,0x44,0x77,0xb8,0xc0,0xe9,0xf6,0xf7,0xa1,0x74,0x27,0x5e,0x10,0x1e,0xf6,0xbf,0x7d,0x18,0xca,0xf0,0x11,0x81,0xd1,0x27,0xb5,0x79};
	 Number privkey(BN_bin2bn(privbuf,sizeof(privbuf),nullptr));
	EC_KEY_set_private_key(keys,privkey);
      constexpr const uint8_t pubbuf[]={0x04,0x51,0x18,0xC3,0x5E,0x9E,0x41,0xE7,0xE0,0x65,0x4F,0xEE,0x80,0x1C,0x52,0xA9,0xC5,0xDF,0xC5,0x10,0xEF,0x09,0x59,0x7D,0x5C,0xCA,0x84,0x61,0xE4,0xAF,0x9C,0x66,0x67,0x14,0x83,0x4F,0x2B,0xC9,0x03,0xF1,0x6F,0xAB,0xFC,0x45,0x75,0x5B,0x01,0x83,0xF1,0xA0,0x97,0x45,0xCD,0xFF,0xCB,0x4E,0x2F,0x79,0x9E,0x50,0xBE,0xD9,0xA6,0xB5,0x8C};
      {
	Point pubkey;
	if(EC_POINT_oct2point(getgroup(),pubkey,pubbuf,sizeof(pubbuf), getBN_CTX())) {
	      LOGAR("EC_POINT_oct2point success");
	      }
      else {
	      LOGAR("EC_POINT_oct2point failure");
	      }

	printPoint("Public key1 ",pubkey);
	EC_KEY_set_public_key(keys,pubkey);
      }
      return keys;
   }
static EC_KEY * getKeyC() {
  static  EC_KEY *keys=mkkeyC();
   return keys;
   }
std::array<uint8_t,64> getchallenge(std::span<const uint8_t> input) {
  std::array<uint8_t,72> sign;
   auto key=getKeyC();
  assert(ECDSA_size(key)==sign.size());
  auto hash= sha256(input.data()+2, 16);
  showhex("digest: ",hash.data(),hash.size());
  unsigned int len= sign.size();
  ECDSA_sign(0, (const uint8_t *)hash.data(), hash.size(), sign.data(), &len, key);
  showhex("Signature: ",sign.data(),len);
  std::array<uint8_t,64> uit{};
  int end1=sign[3];
  int len1;
  if(end1<32) {
      len1=end1;
      end1=32;
      }
    else
      len1=32;
   const auto* startin=sign.data();
   auto* startuit=uit.data();
   std::copy_n(startin+ end1-28,len1,startuit+32-len1);
   int len2=sign[sign[3]+5];
   if(len2>32)
      len2=32;
   std::copy_n(startin+len-len2,len2,startuit+64-len2);
   LOGGER("getchallenge len1=%d end1=%d len2=%d\n",len1,end1,len2);
   showhex("getchallenge ",uit);
  return uit;
   }


#define AES_MAXNR 14
struct aes_key_st {
    unsigned long rd_key[4 * (AES_MAXNR + 1)];
    int rounds;
};
typedef struct aes_key_st AES_KEY;

bool encrypt8AES(const uint8_t *keybytes,const uint8_t *data,unsigned char *uit) {
   unsigned char doubleData[16];
   std::copy_n(data,8,doubleData);
   std::copy_n(data,8,doubleData+8);
   AES_KEY key; 
   showhex("sharedkey: ",keybytes,16);
   if(AES_set_encrypt_keyptr(keybytes, 128, &key))
      return false;
   AES_encryptptr(doubleData, doubleData, &key);
   std::copy_n(doubleData,8,uit);
   return true;
   }
#ifdef TEST

static  byte packby1[]={(byte)0x7C,(byte)0xCC,(byte)0x36,(byte)0xE1,(byte)0x33,(byte)0x64,(byte)0x3A,(byte)0x35,(byte)0x7A,(byte)0x1F,(byte)0xFB,(byte)0xA9,(byte)0xA2,(byte)0xA2,(byte)0x66,(byte)0x24,(byte)0x6E,(byte)0xD5,(byte)0x04,(byte)0x69
,(byte)0x7F,(byte)0x4B,(byte)0xA0,(byte)0x3E,(byte)0x6B,(byte)0x2F,(byte)0x4E,(byte)0x7B,(byte)0x62,(byte)0xB4,(byte)0xBB,(byte)0x88,(byte)0xB4,(byte)0x7E,(byte)0x39,(byte)0x05,(byte)0x2E,(byte)0x0C,(byte)0x11,(byte)0xF5
,(byte)0x25,(byte)0xF3,(byte)0x44,(byte)0xD6,(byte)0xB3,(byte)0xB0,(byte)0x92,(byte)0x4F,(byte)0x3D,(byte)0x33,(byte)0xCC,(byte)0x25,(byte)0x77,(byte)0x5B,(byte)0x8A,(byte)0x55,(byte)0xCD,(byte)0xC6,(byte)0x11,(byte)0x7A
,(byte)0x51,(byte)0x8C,(byte)0xFF,(byte)0x26,(byte)0x2C,(byte)0xC2,(byte)0x26,(byte)0x7B,(byte)0x15,(byte)0x6F,(byte)0x5B,(byte)0xFC,(byte)0x4B,(byte)0xBB,(byte)0xB0,(byte)0xF9,(byte)0x3B,(byte)0xF1,(byte)0xF9,(byte)0xCE
,(byte)0x09,(byte)0xE1,(byte)0x7D,(byte)0x62,(byte)0x13,(byte)0x98,(byte)0xC2,(byte)0xB3,(byte)0x6E,(byte)0x0A,(byte)0xCD,(byte)0x77,(byte)0x2E,(byte)0x71,(byte)0x3A,(byte)0x77,(byte)0xB1,(byte)0x4E,(byte)0x17,(byte)0x5A
,(byte)0xE0,(byte)0x7B,(byte)0x94,(byte)0x34,(byte)0x11,(byte)0x91,(byte)0x8F,(byte)0xCF,(byte)0xED,(byte)0x48,(byte)0x00,(byte)0x66,(byte)0xA4,(byte)0x7C,(byte)0x06,(byte)0xF4,(byte)0xC2,(byte)0x5B,(byte)0x01,(byte)0xCB
,(byte)0x20,(byte)0xB1,(byte)0x48,(byte)0xC0,(byte)0x36,(byte)0x81,(byte)0x9F,(byte)0x4A,(byte)0xFE,(byte)0xD6,(byte)0xF7,(byte)0xAA,(byte)0xF7,(byte)0xDF,(byte)0xCF,(byte)0xBC,(byte)0xF0,(byte)0x96,(byte)0x5A,(byte)0xE8
,(byte)0xE1,(byte)0x19,(byte)0x00,(byte)0x02,(byte)0x2E,(byte)0x92,(byte)0x98,(byte)0xB6,(byte)0xA5,(byte)0x46,(byte)0xB1,(byte)0x47,(byte)0x69,(byte)0xCB,(byte)0xFE,(byte)0xE1,(byte)0xC7,(byte)0x7B,(byte)0x91,(byte)0x70 };
static  byte packby2[]={(byte)0x0B,(byte)0x7D,(byte)0x5B,(byte)0xC6,(byte)0x78,(byte)0xF0,(byte)0x18,(byte)0xF2,(byte)0xD0,(byte)0xD8,(byte)0x6E,(byte)0xF4,(byte)0xB9,(byte)0x82,(byte)0x81,(byte)0x3E,(byte)0x7F,(byte)0x50,(byte)0x1C,(byte)0x0D
,(byte)0x14,(byte)0x29,(byte)0x75,(byte)0xEF,(byte)0xDA,(byte)0x08,(byte)0xE5,(byte)0x39,(byte)0xDB,(byte)0xF8,(byte)0xE0,(byte)0x4D,(byte)0x0A,(byte)0xB6,(byte)0xFD,(byte)0x61,(byte)0x1D,(byte)0xBC,(byte)0xFE,(byte)0x1B
,(byte)0xAF,(byte)0xD4,(byte)0x6A,(byte)0x2F,(byte)0xB8,(byte)0x06,(byte)0x64,(byte)0x0C,(byte)0x75,(byte)0x87,(byte)0x2A,(byte)0x21,(byte)0x86,(byte)0xB7,(byte)0x47,(byte)0xA6,(byte)0xAF,(byte)0xB8,(byte)0xBE,(byte)0xA7
,(byte)0x21,(byte)0xE3,(byte)0x81,(byte)0xBF,(byte)0x82,(byte)0x3E,(byte)0x7B,(byte)0xE9,(byte)0xBE,(byte)0x45,(byte)0x75,(byte)0x7C,(byte)0x21,(byte)0x9F,(byte)0x6A,(byte)0x9F,(byte)0x0F,(byte)0x5D,(byte)0x2D,(byte)0x9D
,(byte)0xE0,(byte)0x1C,(byte)0xD0,(byte)0x5D,(byte)0x3D,(byte)0x72,(byte)0xC9,(byte)0x11,(byte)0xD0,(byte)0xBA,(byte)0xE2,(byte)0x2C,(byte)0x48,(byte)0xEF,(byte)0x05,(byte)0x71,(byte)0x7A,(byte)0xD3,(byte)0xFC,(byte)0x96
,(byte)0x2B,(byte)0xC4,(byte)0x79,(byte)0x15,(byte)0xF9,(byte)0x83,(byte)0x28,(byte)0x5C,(byte)0x4B,(byte)0x78,(byte)0x17,(byte)0x4B,(byte)0xE1,(byte)0xD6,(byte)0x31,(byte)0x51,(byte)0x72,(byte)0x5D,(byte)0xEC,(byte)0x83
,(byte)0x4C,(byte)0x4C,(byte)0xF0,(byte)0x76,(byte)0x9B,(byte)0x44,(byte)0xF8,(byte)0x36,(byte)0x7D,(byte)0xFF,(byte)0xB9,(byte)0x61,(byte)0xD2,(byte)0xA1,(byte)0x74,(byte)0xBF,(byte)0x3F,(byte)0x81,(byte)0x48,(byte)0x70
,(byte)0x7E,(byte)0x5D,(byte)0xAE,(byte)0x97,(byte)0x4A,(byte)0xDF,(byte)0xFB,(byte)0x3F,(byte)0x41,(byte)0xC3,(byte)0xE3,(byte)0x78,(byte)0xA8,(byte)0xC4,(byte)0x4D,(byte)0x86,(byte)0x66,(byte)0x16,(byte)0x8E,(byte)0xF3};

void testmulti() {

   auto bn_ctx=getBN_CTX();
   const auto group=getgroup();


   Number  private_key;
   BN_hex2bn(&private_key.data,"54fd40eafbe36079e92056a79b7b69c672fb35452179a3f3a30c00402c4a71c3");
   printNum("priv_keyA: ",private_key);
    Point pub_keyA;
   EC_POINT_mul(group,pub_keyA,private_key,nullptr,nullptr,bn_ctx);

    printPoint("pub_keyA: ",pub_keyA);

   Number  p,a,b; 
//   ec_group_get_field(group, field);
   EC_GROUP_get_curve(group, p,  a, b, nullptr);
   printNum("p=",p);
   printNum("a=",a);
   printNum("b=",b);
  // const BIGNUM *cofactor=EC_GROUP_get0_cofactor(group);
   Number cofactor;
   EC_GROUP_get_cofactor(group,cofactor,bn_ctx);
   printNum("cofactor",cofactor);

   Number ran1;
   BN_hex2bn(&ran1.data,"fbc271b637e2491e45a4179ed33665c506a1e0a1d350f5af0f96370695fdc323");
   const auto g=EC_GROUP_get0_generator(getgroup());
   printPoint("g: ", g);
PCert pack1;
pack1.frombytes(packby1);
LOGGER("pack1 is %svalid\n",validate12(pack1)?"":"un");
 auto b1=pack1.byteify();
  if(std::equal(std::begin(packby1),std::end(packby1),std::begin(b1))) {
   LOGAR("packby1 equal");
   }
  else
   LOGAR("packby1 unequal");
PCert pack2;
pack2.frombytes(packby2);

LOGGER("pack2 is %svalid\n",validate12(pack2)?"":"un");
 auto b2=pack2.byteify();

  if(std::equal(std::begin(packby2),std::end(packby2),std::begin(b2))) {
   LOGAR("packby2 equal");
   }
  else
   LOGAR("packby2 unequal");
  auto packet= mkround12(g,pub_keyA,  private_key, ran1);
   showhex("packet1: ",packet.data(),packet.size());
uchar privbbuf[]={0x95,0xae,0x54,0xcd,0x1f,0x15,0x42,0xb9,0xaa,0x55,0xdf,0x0b,0x24,0x6e,0xc9,0xb9,0xac,0xd4,0x16,0x68,0xda,0x8e,0xd3,0xc1,0x34,0x24,0x90,0x79,0x48,0xa9,0xd1,0x8f};
  Number privB(BN_bin2bn(privbbuf,sizeof(privbbuf),nullptr));
    Point pub_keyB;
   EC_POINT_mul(group,pub_keyB,privB,nullptr,nullptr,bn_ctx);
    printPoint("pub_keyB: ",pub_keyB);

   Number ran2;
   BN_hex2bn(&ran2.data,"fbc271b8b7e2491e45a4179edbb865c508a1e0a1db50f5af0f96b70695fdcb2b");
  auto packet2= mkround12(g,pub_keyB,  privB, ran2);
   showhex("packet2: ",packet2.data(),packet2.size());


uchar passbuf[]="1155";
  Number pass(BN_bin2bn(passbuf,4,nullptr));
//auto pack3=makeRound3Cert(pub1,pub2,pub_keyA,privB, pass);

PCert pack3;
makeRound3Cert(pack3,pack1.pubkey1,pack2.pubkey1,pub_keyA,privB, pass);

validate3(pub_keyA,pub_keyB,pack1,pack3);
auto byte3=pack3.byteify(); 
showhex("byte3: ",byte3.data(),byte3.size());

auto key=mkSharedKey(pack2,pack3,pass,privB) ;
std::array<uint8_t,16> sharedKey;
//std::copy(key.data(),&key[16],sharedKey.data());
std::copy_n(key.data(),16,sharedKey.data());
showhex("sharedKey: ",sharedKey);

}

int main() {
   if(!hasCrypto()) {
   	LOGAR("no libcrypto.so");
	return -1;
   	}
  testmulti();

  const uint8_t challenge[]=
{
(uint8_t)0x0C,(uint8_t)0x00,(uint8_t)0x0C,(uint8_t)0xEE,(uint8_t)0x69,(uint8_t)0x1B,(uint8_t)0x76,(uint8_t)0x5A,(uint8_t)0x49,(uint8_t)0x7D,(uint8_t)0x22,(uint8_t)0x58,(uint8_t)0x23,(uint8_t)0xD1,(uint8_t)0x4F,(uint8_t)0x27,(uint8_t)0x8D,(uint8_t)0xD3
}; 
   auto out=getchallenge(challenge);
   unsigned char key[]={ 0x6f,0x83,0x26,0x74,0x4b,0xef,0x03,0xfa,0xa5,0x20,0xad,0x9c,0x5c,0xff,0x67,0x3f};
   unsigned char data[]={ 0x2A,0x40,0x42,0x90,0xC4,0xB6,0x3B,0x01};

   uint8_t uit[8];
 if(encrypt8AES(key,data,uit)) {
   showhex("encrypt8AES: ",uit,8);
   }
  else
   LOGAR("encrypt8AES failed");

   }
#endif

#ifndef TEST
#include "SensorGlucoseData.hpp"
#include "DexContext.hpp"
DexContext::DexContext(SensorGlucoseData *sens): pin(hasCrypto()?BN_bin2bn(sens->getDexPin().data(),4,nullptr):nullptr) { }
DexContext::~DexContext() { BN_free(const_cast<BIGNUM*>(pin)); }
#endif
#endif
