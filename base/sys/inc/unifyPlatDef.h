#ifndef __BASE_SYS_INC_UNIFYPLATDEF_H__
#define __BASE_SYS_INC_UNIFYPLATDEF_H__

#if defined (_WIN32)

#include <windef.h>
#include <winsock2.h>

typedef SOCKET sockhdl;
typedef HANDLE filehdl;

#elif defined (__linux__) || defined(__APPLE__)

typedef int sockhdl;
typedef int filehdl;

#endif

#endif // __BASE_SYS_INC_UNIFYPLATDEF_H__
