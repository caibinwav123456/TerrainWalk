#ifndef _COMMON_H_
#define _COMMON_H_
#ifndef NULL
#define NULL 0
#endif
#define VALID(M) ((M)==0?false:((M)==(void*)-1?((M)=0,false):true))
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
#define hiword(x) ((x)>>16)
#define loword(x) ((x)&0xffff)
#if (defined(DEBUG) || defined(_DEBUG)) && !defined(NDEBUG)
#define verify(m) assert(m)
#else
#define verify(m) (m)
#endif
#define DLL
#define DLLAPI(T) DLL T
#include "ASTError.h"
#include "defines.h"
#include "sys.h"
#endif