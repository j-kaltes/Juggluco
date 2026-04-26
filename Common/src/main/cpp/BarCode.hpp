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
/*      Fri Apr 03 10:40:38 CEST 2026                                                */
#include <unordered_map>
#include <vector>
#include <array>
#include <string_view>
#include <stdexcept>
#include <cctype>
#include <stdio.h>
#ifndef LOGGER
#define LOGGER(...) fprintf(stdout,__VA_ARGS__)
#define LOGAR(...) puts(__VA_ARGS__)
#endif
using namespace std::literals;
static inline bool isDigit(char c) { return c >= '0' && c <= '9'; }

// Convert scanner representation "^]" into real GS (0x1D) and strip a leading "^]" if present.
/*
#include <string>
static std::string normalizeGs(const std::string_view s) {
  std::string out;
  out.reserve(s.size());

  for (size_t i = 0; i < s.size(); ) {
    if (i + 1 < s.size() && s[i] == '^' && s[i + 1] == ']') {
      out.push_back(static_cast<char>(0x1D));
      i += 2;
    } else {
      out.push_back(s[i++]);
    }
  }
  // Some scanners prepend GS at start. It's fine to keep it, but we can drop leading GS for simplicity.
  if (!out.empty() && static_cast<unsigned char>(out[0]) == 0x1D) out.erase(out.begin());
  return out;
}
*/
// Fixed lengths for common UDI fields (extend as needed).

struct BarCode {
 //   std::string_view SSCC;
    std::string_view GTIN;
    std::string_view ProductionDate;
    std::string_view Expiry;
    std::string_view Lot;
    std::string_view Serial;
    std::string_view PIN;
    std::string_view SensorCode;

   struct GTINtype {
        char baseconsumerunnit;
        std::array<char,7> manifacturer;
        std::array<char,5> SKU;
        char errorDetection;
        };
    std::array<char,7> getManifacturer() const {
        GTINtype *gtin=(GTINtype*)GTIN.data();
        if(!gtin)
                return {}; 
        return gtin->manifacturer;
        }
    std::array<char,5> getSKU() const {
        GTINtype *gtin=(GTINtype*)GTIN.data();
        if(!gtin)
                return {}; 
        return gtin->SKU;
        }

    struct el {
        std::string_view *data;
        int size;
        bool fixed;
        };
    const std::unordered_map<std::string_view, el> fields = {
    //  {"00", 18}, // SSCC
      {"01", {&GTIN,14,true}}, // GTIN
      {"11", {&ProductionDate,6,true}},  // Production date YYMMDD
     // {"12", 6},  // Due date YYMMDD
    //  {"13", 6},  // Packaging date YYMMDD
     // {"15", 6},  // Best before YYMMDD
      {"17", {&Expiry,6,true}},  // Expiration YYMMDD
    //  {"20", 2},  // Variant
      {"10", {&Lot,20,false}},  // Lot/batch
      {"21", {&Serial,20,false}},  // Serial
      {"240", {&PIN,30,false}}, // Additional product identification
    //  {"241", 30}, // Customer part number
    //  {"242", 6},  // Made-to-order variation number
      {"250", {&SensorCode,30,false}} // Secondary serial
     // {"251", 30}, // Reference to source entity
    };
// Attempt to read an AI starting at pos. Returns AI string and advances pos.
// Strategy: try 4-digit, then 3-digit, then 2-digit, but only if it matches known AIs.
auto readAI(const std::string_view s, size_t& pos) {
  if (pos >= s.size()) throw std::runtime_error("Unexpected end while reading AI");
  if (!isDigit(s[pos])) throw std::runtime_error("Expected digit at AI start");
  typedef decltype(fields.end()) RetType;
  // Try 4-digit AI
  if (pos + 4 <= s.size()) {
    std::string_view ai4 = s.substr(pos, 4);
    if(auto res=fields.find(ai4);res!=fields.end()) {
         pos += 4; 
         return (RetType)res; 
        }
  }
  // Try 3-digit AI
  if (pos + 3 <= s.size()) {
    std::string_view ai3 = s.substr(pos, 3);
    if(auto res=fields.find(ai3);res!=fields.end()) {
         pos += 3; 
         return (RetType)res; 
        }
  }
  // Fallback 2-digit AI
  if (pos + 2 <= s.size()) {
    std::string_view ai2 = s.substr(pos, 2);
    if(auto res=fields.find(ai2);res!=fields.end()) { 
        pos += 2; 
         return (RetType)res; 
        }
  }
return fields.end();
}
static std::string_view readValue(const std::string_view s, size_t& pos, const int fl) {
  if (fl > 0) {
    if (pos + static_cast<size_t>(fl) > s.size()) throw std::runtime_error("Truncated fixed-length field");
    std::string_view v = s.substr(pos, fl);
    pos += fl;
    return v;
  }
  // Variable-length: read until GS (0x1D) or end.
  size_t start = pos;
  while (pos < s.size() && static_cast<unsigned char>(s[pos]) != 0x1D) pos++;
  std::string_view v = s.substr(start, pos - start);

  // If we're at GS, consume it and continue (it terminates this variable field).
  if (pos < s.size() && static_cast<unsigned char>(s[pos]) == 0x1D) pos++;
  return v;
}
public:
void showscan() const {
#ifndef NOLOG
  const BarCode &barcode=*this;
  LOGGER("GTIN: %.*s\n",(int)barcode.GTIN.size(),barcode.GTIN.data());
  LOGGER("Manifacturer: %.*s\n",(int)barcode.getManifacturer().size(),barcode.getManifacturer().data());
  LOGGER("SKU: %.*s\n",(int)barcode.getSKU().size(),barcode.getSKU().data());
  LOGGER("ProductionDate: %.*s\n",(int)barcode.ProductionDate.size(),barcode.ProductionDate.data());
  LOGGER("Expiry: %.*s\n",(int)barcode.Expiry.size(),barcode.Expiry.data());
  LOGGER("Lot: %.*s\n",(int)barcode.Lot.size(),barcode.Lot.data());
  LOGGER("Serial: %.*s\n",(int)barcode.Serial.size(),barcode.Serial.data());
  LOGGER("PIN: %.*s\n",(int)barcode.PIN.size(),barcode.PIN.data());
  LOGGER("SensorCode: %.*s\n\n",(int)barcode.SensorCode.size(),barcode.SensorCode.data());
#endif
}
bool parseGs1(const std::string_view raw) {
//  std::string s = normalizeGs(raw);
    std::string_view s=raw.substr(1);

  size_t pos = 0;

  while (pos < s.size()) {
    // Skip any stray GS (robustness)
    if (static_cast<unsigned char>(s[pos]) == 0x1D) { pos++; continue; }

    auto el = readAI(s, pos);
    if(el==fields.end()) {
        return false;
        }
    auto dat=el->second;
    int fl = dat.fixed?dat.size:-1;
    std::string_view value = readValue(s, pos,fl );
    *dat.data=value;
  
    // Optional: validate max length for variable fields
    if(!dat.fixed&&value.size()>dat.size) {
        LOGGER("Warning %*.s  larger than %d\n",(int)value.size(),value.data(),dat.size);
        }
    }
     return true;
    }
    } ;



// Read a value for a given AI starting at pos.
//static std::string_view readValue(const std::string_view s, size_t& pos, const std::string_view ai) {


/*
std::cout<<"EU sibionics"<<std::endl;
const auto eu{"0106972831641803112412191725121810LT4F241247J21241247YEZ1450HAJ02"sv};
showscan(eu);
std::cout<<
//0106972831641117112406121725061110LT48240601R21240601YL08230BFY73 Hematoxic
//0106972831641117112406121725061110LT48240601R21240601YL08230BFY73 Hematoxic
//0106972831641476112412231725122210LT46241219C21WD9QAXGA52WS4V   len=63 Sibionics light
//0106972831641476112504081726040710LT46250316C21P2250316015APD66 len=65 Sibionics 2
//^]0106972831640165112312091724120810LT41231108C^]21231108GEPD802JPP76  SIBIONICS  name 31108GEPD802JPP7
//^]0106972831641483112411201726051910LT46241155C^]21P22411J6EP  SIBIONICS2 transmitter
*/
