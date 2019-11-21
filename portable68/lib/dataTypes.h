
#ifndef DATA_TYPES_H
#define DATA_TYPES_H

typedef signed char i8;
typedef unsigned char u8;
typedef signed long long i64;
typedef unsigned long long u64;

template<int> struct Type {};

template<> struct Type< 1  > { typedef signed   int   i16; };
template<> struct Type< 2  > { typedef signed   short i16; };
template<> struct Type< 3  > { typedef signed   long  i16; };
template<> struct Type< 4  > { typedef unsigned int   u16; };
template<> struct Type< 5  > { typedef unsigned short u16; };
template<> struct Type< 6  > { typedef unsigned long  u16; };
template<> struct Type< 7  > { typedef signed   int   i32; };
template<> struct Type< 8  > { typedef signed   short i32; };
template<> struct Type< 9  > { typedef signed   long  i32; };
template<> struct Type< 10 > { typedef unsigned int   u32; };
template<> struct Type< 11 > { typedef unsigned short u32; };
template<> struct Type< 12 > { typedef unsigned long  u32; };

typedef Type<sizeof(int) == 2 ?  1 : sizeof(short) == 2 ?  2 : sizeof(long) == 2 ?  3 : 0>::i16 i16;
typedef Type<sizeof(int) == 2 ?  4 : sizeof(short) == 2 ?  5 : sizeof(long) == 2 ?  6 : 0>::u16 u16;
typedef Type<sizeof(int) == 4 ?  7 : sizeof(short) == 4 ?  8 : sizeof(long) == 4 ?  9 : 0>::i32 i32;
typedef Type<sizeof(int) == 4 ? 10 : sizeof(short) == 4 ? 11 : sizeof(long) == 4 ? 12 : 0>::u32 u32;

#endif
