#ifndef __BASE_SYS_INC_UNIFYPLATDEF_H__
#define __BASE_SYS_INC_UNIFYPLATDEF_H__

#if defined (_WIN32)

#include <windef.h>
#include <winsock2.h>

typedef sockhdl SOCKET;
typedef filehdl HANDLE;

#elif defined (__linux__) || defined(__APPLE__)

typedef sockhdl int;
typedef filehdl int;

#endif

#endif // __BASE_SYS_INC_UNIFYPLATDEF_H__
