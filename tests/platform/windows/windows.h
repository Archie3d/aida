#ifndef AIDA_TEST_WINDOWS_H
#define AIDA_TEST_WINDOWS_H

#define THREAD_PRIORITY_IDLE (-15)
#define THREAD_PRIORITY_LOWEST (-2)
#define THREAD_PRIORITY_BELOW_NORMAL (-1)
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_HIGHEST 2

#include <stddef.h>
#include <stdint.h>

/* Minimal Win32 surface for the host-side runtime branch contract test.
   Native Windows CI compiles against the real SDK and winpthreads. */
typedef struct SYSTEM_INFO
{
    uint32_t dwPageSize;
    uint32_t dwAllocationGranularity;
} SYSTEM_INFO;
typedef struct IMAGE_DOS_HEADER
{
    uint16_t e_magic;
    int32_t e_lfanew;
} IMAGE_DOS_HEADER;
typedef struct IMAGE_NT_HEADERS
{
    uint32_t Signature;
    struct
    {
        uint64_t SizeOfStackReserve;
    } OptionalHeader;
} IMAGE_NT_HEADERS;
#define IMAGE_DOS_SIGNATURE 0x5a4d
#define IMAGE_NT_SIGNATURE 0x4550
#define THREAD_PRIORITY_TIME_CRITICAL 15

void GetSystemInfo(SYSTEM_INFO* information);
void* GetModuleHandleW(const wchar_t* name);

#endif
