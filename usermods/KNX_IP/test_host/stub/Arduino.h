#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <string>
#include <functional>
#include <algorithm>
#include <ctime>
#include <cstdarg>
using std::isnan;
#define HEX 16
#define DEC 10
typedef uint8_t byte;
struct String { std::string s; String(int){} String(unsigned){} String(long){} String(unsigned long){} String(float,int=2){} String(const std::string&x):s(x){} bool endsWith(const char*) const {return false;} bool startsWith(const char*) const {return false;} int indexOf(const char*) const {return -1;} int indexOf(char) const {return -1;} String substring(int,int=-1) const {return *this;} int toInt() const {return 0;} void reserve(size_t){} String operator+(const String&o) const {return String(s+o.s);} String operator+(const char*o) const {return String(s+o);} String& operator+=(const String&o){s+=o.s;return *this;} String& operator+=(char c){s+=c;return *this;} String& operator+=(int){return *this;} String& operator+=(unsigned){return *this;} String& operator+=(long){return *this;} String& operator+=(unsigned long){return *this;} bool operator==(const String&o) const {return s==o.s;} bool operator==(const char*o) const {return s==o;} char operator[](size_t i) const {return s[i];} void trim(){} bool isEmpty() const {return s.empty();}  String(){} String(const char*c):s(c){} size_t length() const {return s.size();} const char* c_str() const {return s.c_str();} String& operator+=(const char*c){s+=c;return *this;} bool operator!=(const String&o) const {return s!=o.s;} };
struct Print { virtual size_t print(const char*){return 0;} };
struct HardwareSerial { int printf(const char*, ...){return 0;} template<class T> void print(T,int=0){} template<class T> void println(T,int=0){} void println(){} };
extern HardwareSerial Serial;
inline String operator+(const char*a,const String&b){return String(std::string(a)+b.s);}
using std::min; using std::max;
#define F(x) x
unsigned long millis();
void delay(unsigned long);
void yield();
float temperatureRead();
struct IPAddress { uint32_t a=0; uint8_t operator[](int) const {return 0;} operator uint32_t() const {return a;} IPAddress(){} IPAddress(int,int,int,int){} String toString() const {return String("0");} bool operator!=(const IPAddress&o) const {return a!=o.a;} };
