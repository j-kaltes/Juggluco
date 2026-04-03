/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Fri Apr 03 10:43:24 CEST 2026                                                */
#include <dlfcn.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

#include "logs.hpp"
#include "cryptoextern.h"
#include "cryptodecl.h"
extern void* opencrypto();

static bool initcrypto() {
   LOGAR("start initAidexXcrypto");
#define hgetsym(handle,name) *((void **)&name##ptr)=dlsym(handle, #name)
#define getsym(name) hgetsym(handle,name)
#define symtest(name) if(!name##ptr&&!(getsym(name))) { LOGGER(#name ": %s\n",dlerror());}
   void *handle=opencrypto();
   if(!handle)  {
        LOGAR("opencrypto() failed");
        return false;
        }
    #include "cryptosyms.h"
 if(!EVP_MD_CTX_freeptr) {
     if(!( *((void **)& EVP_MD_CTX_freeptr)=dlsym(handle, "EVP_MD_CTX_destroy"))) {
         LOGGER("EVP_MD_CTX_destroy: %s\n",dlerror());
         return false;
         }
      }
   if(!EVP_MD_CTX_newptr) {
     if(!( *((void **)& EVP_MD_CTX_newptr)=dlsym(handle, "EVP_MD_CTX_create"))) {
     LOGGER("EVP_MD_CTX_create: %s\n",dlerror());
     return false;
     }
   }

   LOGAR("end initAidexXcrypto");
   return true;
   }

extern bool initAidexXcrypto();
bool initAidexXcrypto() {
        static bool initialized= initcrypto();
        return initialized;
        }

