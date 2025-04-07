// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WIN32

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include "Engine/Core/Log.h"
#include "IncludeWindowsHeaders.h"
#include <Psapi.h>
#include <WinSock2.h>
#include <IPHlpApi.h>
#include <oleauto.h>
#include <WinBase.h>
#include <xmmintrin.h>
#include <intrin.h>
#pragma comment(lib, "Iphlpapi.lib")

static_assert(sizeof(int32) == sizeof(long), "Invalid long size for Interlocked and Atomic operations in Win32Platform.");

namespace
{
    Guid DeviceId;
    CPUInfo CpuInfo;
    uint64 ProgramSizeMemory;
    uint64 ClockFrequency;
    double CyclesToSeconds;
    WSAData WsaData;
}

// Helper function to count set bits in the processor mask
DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = static_cast<ULONG_PTR>(1) << LSHIFT;
    DWORD i;

    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest) ? 1 : 0);
        bitTest /= 2;
    }

    return bitSetCount;
}

static String GetLastErrorMessage()
{
    wchar_t* s = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, WSAGetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   reinterpret_cast<LPWSTR>(&s), 0, nullptr);
    String str(s);
    LocalFree(s);
    return str;
}

bool Win32Platform::Init()
{
    if (PlatformBase::Init())
        return true;

    // Init timing
    LARGE_INTEGER frequency;
    const auto freqResult = QueryPerformanceFrequency(&frequency);
    ASSERT(freqResult && frequency.QuadPart > 0);
    ClockFrequency = frequency.QuadPart;
    CyclesToSeconds = 1.0 / static_cast<double>(frequency.QuadPart);

	// Estimate program size by checking physical memory usage on start
	ProgramSizeMemory = Platform::GetProcessMemoryStats().UsedPhysicalMemory;

    // Count CPUs
    BOOL done = FALSE;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr;
    DWORD returnLength = 0;
    DWORD logicalProcessorCount = 0;
    DWORD processorCoreCount = 0;
    DWORD processorL1CacheSize = 0;
    DWORD processorL2CacheSize = 0;
    DWORD processorL3CacheSize = 0;
    DWORD processorPackageCount = 0;
    DWORD byteOffset = 0;
    PCACHE_DESCRIPTOR cache;
    while (!done)
    {
        DWORD rc = GetLogicalProcessorInformation(buffer, &returnLength);
        if (rc == FALSE)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                if (buffer)
                {
                    free(buffer);
                }
                buffer = static_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(malloc(returnLength));
                if (buffer == nullptr)
                {
                    return true;
                }
            }
            else
            {
                return true;
            }
        }
        else
        {
            done = TRUE;
        }
    }
    ptr = buffer;
    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
    {
        switch (ptr->Relationship)
        {
        case RelationProcessorCore:
            processorCoreCount++;
            logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
            break;
        case RelationCache:
            cache = &ptr->Cache;
            if (cache->Level == 1)
            {
                processorL1CacheSize += cache->Size;
            }
            else if (cache->Level == 2)
            {
                processorL2CacheSize += cache->Size;
            }
            else if (cache->Level == 3)
            {
                processorL3CacheSize += cache->Size;
            }
            break;
        case RelationProcessorPackage:
            processorPackageCount++;
            break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }
    free(buffer);

    // Set info about the CPU
    CpuInfo.ProcessorPackageCount = processorPackageCount;
    CpuInfo.ProcessorCoreCount = processorCoreCount;
    CpuInfo.LogicalProcessorCount = logicalProcessorCount;
    CpuInfo.L1CacheSize = processorL1CacheSize;
    CpuInfo.L2CacheSize = processorL2CacheSize;
    CpuInfo.L3CacheSize = processorL3CacheSize;
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    CpuInfo.PageSize = siSysInfo.dwPageSize;
    CpuInfo.ClockSpeed = ClockFrequency;
    {
#ifdef _M_ARM64
        CpuInfo.CacheLineSize = 128;
#else
        int args[4];
        __cpuid(args, 0x80000006);
        CpuInfo.CacheLineSize = args[2] & 0xFF;
        ASSERT(CpuInfo.CacheLineSize && Math::IsPowerOfTwo(CpuInfo.CacheLineSize));
#endif
    }

    // Setup unique device ID
    {
        DeviceId = Guid::Empty;

        // A - Computer Name and User Name
        uint32 hash = GetHash(Platform::GetComputerName());
        CombineHash(hash, GetHash(Platform::GetUserName()));
        DeviceId.A = hash;

        // B - MAC address
        DeviceId.B = 0;
        IP_ADAPTER_ADDRESSES pAddresses[16];
        DWORD dwBufLen = sizeof(pAddresses);
#if PLATFORM_UWP
	    DWORD dwStatus = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &dwBufLen);
	    PIP_ADAPTER_ADDRESSES pAdapterInfo = dwStatus == ERROR_SUCCESS ? pAddresses : nullptr;
	    while (pAdapterInfo)
	    {
		    hash = pAdapterInfo->PhysicalAddress[0];
		    for (UINT i = 1; i < pAdapterInfo->PhysicalAddressLength; i++)
			    CombineHash(hash, pAdapterInfo->PhysicalAddress[i]);
		    DeviceId.B = hash;

		    pAdapterInfo = pAdapterInfo->Next;
	    }
#else
        const DWORD dwStatus = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &dwBufLen);
        PIP_ADAPTER_ADDRESSES pAdapterInfo = dwStatus == ERROR_SUCCESS ? pAddresses : nullptr;
        while (pAdapterInfo)
        {
            hash = pAdapterInfo->PhysicalAddress[0];
            for (UINT i = 1; i < pAdapterInfo->PhysicalAddressLength; i++)
                CombineHash(hash, pAdapterInfo->PhysicalAddress[i]);
            DeviceId.B = hash;

            pAdapterInfo = pAdapterInfo->Next;
        }
#endif

        // C - memory
        DeviceId.C = (uint32)Platform::GetMemoryStats().TotalPhysicalMemory;

        // D - cpuid
        auto cpuInfo = Platform::GetCPUInfo();
        DeviceId.D = (uint32)cpuInfo.ClockSpeed * cpuInfo.LogicalProcessorCount * cpuInfo.ProcessorCoreCount * cpuInfo.CacheLineSize;
    }

    // Init networking
    if (WSAStartup(MAKEWORD(2, 0), &WsaData) != 0)
        LOG(Error, "Unable to initializes native network! Error : {0}", GetLastErrorMessage());

    return false;
}

void Win32Platform::Exit()
{
    WSACleanup();
}

void Win32Platform::MemoryBarrier()
{
    _ReadWriteBarrier();
#if PLATFORM_64BITS
#if defined(_AMD64_)
    __faststorefence();
#elif defined(_IA64_)
	__mf();
#elif defined(_ARM64_)
    __dmb(_ARM64_BARRIER_ISH);
#else
#error "Invalid platform."
#endif
#else
	LONG barrier;
	__asm {
		xchg barrier, eax
	}
#endif
}

void Win32Platform::Prefetch(void const* ptr)
{
#if _M_ARM64
    __prefetch((char const*)ptr);
#else
    _mm_prefetch((char const*)ptr, _MM_HINT_T0);
#endif
}

void* Win32Platform::Allocate(uint64 size, uint64 alignment)
{
    void* ptr = _aligned_malloc((size_t)size, (size_t)alignment);
    if (!ptr)
        OutOfMemory();
#if COMPILE_WITH_PROFILER
    OnMemoryAlloc(ptr, size);
#endif
    return ptr;
}

void Win32Platform::Free(void* ptr)
{
#if COMPILE_WITH_PROFILER
    OnMemoryFree(ptr);
#endif
    _aligned_free(ptr);
}

void* Win32Platform::AllocatePages(uint64 numPages, uint64 pageSize)
{
    const uint64 numBytes = numPages * pageSize;
#if PLATFORM_UWP
    return VirtualAllocFromApp(nullptr, (SIZE_T)numBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    return VirtualAlloc(nullptr, (SIZE_T)numBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#endif
}

void Win32Platform::FreePages(void* ptr)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

bool Win32Platform::Is64BitPlatform()
{
#ifdef PLATFORM_64BITS
    return true;
#else
	BOOL result;
	IsWow64Process(GetCurrentProcess(), &result);
	return result == TRUE;
#endif
}

CPUInfo Win32Platform::GetCPUInfo()
{
    return CpuInfo;
}

MemoryStats Win32Platform::GetMemoryStats()
{
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    MemoryStats result;
    result.TotalPhysicalMemory = statex.ullTotalPhys;
    result.UsedPhysicalMemory = statex.ullTotalPhys - statex.ullAvailPhys;
    result.TotalVirtualMemory = statex.ullTotalVirtual;
    result.UsedVirtualMemory = statex.ullTotalVirtual - statex.ullAvailVirtual;
    result.ProgramSizeMemory = ProgramSizeMemory;
    return result;
}

ProcessMemoryStats Win32Platform::GetProcessMemoryStats()
{
    PROCESS_MEMORY_COUNTERS_EX countersEx;
    countersEx.cb = sizeof(countersEx);
    GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&countersEx, sizeof(countersEx));
    ProcessMemoryStats result;
    result.UsedPhysicalMemory = countersEx.WorkingSetSize;
    result.UsedVirtualMemory = countersEx.PrivateUsage;
    return result;
}

uint64 Win32Platform::GetCurrentProcessId()
{
    return ::GetCurrentProcessId();
}

void Win32Platform::SetThreadPriority(ThreadPriority priority)
{
    int32 winPriority;
    switch (priority)
    {
    case ThreadPriority::Lowest:
        winPriority = THREAD_PRIORITY_LOWEST;
        break;
    case ThreadPriority::BelowNormal:
        winPriority = THREAD_PRIORITY_BELOW_NORMAL;
        break;
    case ThreadPriority::Normal:
        winPriority = THREAD_PRIORITY_NORMAL;
        break;
    case ThreadPriority::AboveNormal:
        winPriority = THREAD_PRIORITY_ABOVE_NORMAL;
        break;
    case ThreadPriority::Highest:
        winPriority = THREAD_PRIORITY_HIGHEST;
        break;
    default:
        winPriority = THREAD_PRIORITY_NORMAL;
        break;
    }
    ::SetThreadPriority(::GetCurrentThread(), winPriority);
}

void Win32Platform::SetThreadAffinityMask(uint64 affinityMask)
{
    ::SetThreadAffinityMask(::GetCurrentThread(), (DWORD_PTR)affinityMask);
}

void Win32Platform::Sleep(int32 milliseconds)
{
    static thread_local HANDLE timer = NULL;
    if (timer == NULL)
    {
        // Attempt to create high-resolution timer for each thread (Windows 10 build 17134 or later)
        timer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        if (timer == NULL) // fallback for older versions of Windows
            timer = CreateWaitableTimer(NULL, TRUE, NULL);
    }

    // Negative value is relative to current time, minimum waitable time is 10 microseconds
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -int64_t(milliseconds) * 10000;

    SetWaitableTimerEx(timer, &dueTime, 0, NULL, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
}

double Win32Platform::GetTimeSeconds()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return double(counter.QuadPart) * CyclesToSeconds;
}

uint64 Win32Platform::GetTimeCycles()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

uint64 Win32Platform::GetClockFrequency()
{
    return ClockFrequency;
}

void Win32Platform::GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    // Get current local time
    SYSTEMTIME st;
    ::GetLocalTime(&st);

    // Extract time
    year = st.wYear;
    month = st.wMonth;
    dayOfWeek = st.wDayOfWeek;
    day = st.wDay;
    hour = st.wHour;
    minute = st.wMinute;
    second = st.wSecond;
    millisecond = st.wMilliseconds;
}

void Win32Platform::GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    // Get current system time
    SYSTEMTIME st;
    ::GetSystemTime(&st);

    // Extract time
    year = st.wYear;
    month = st.wMonth;
    dayOfWeek = st.wDayOfWeek;
    day = st.wDay;
    hour = st.wHour;
    minute = st.wMinute;
    second = st.wSecond;
    millisecond = st.wMilliseconds;
}

void Win32Platform::CreateGuid(Guid& result)
{
    CoCreateGuid(reinterpret_cast<GUID*>(&result));
}

String Win32Platform::GetMainDirectory()
{
    Char buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    const String str(buffer);
    int32 pos = str.FindLast(TEXT('\\'));
    if (pos != -1 && ++pos < str.Length())
        return str.Left(pos);
    return str;
}

String Win32Platform::GetExecutableFilePath()
{
    Char buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return String(buffer);
}

Guid Win32Platform::GetUniqueDeviceId()
{
    return DeviceId;
}

String Win32Platform::GetWorkingDirectory()
{
    Char buffer[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, buffer);
    return String(buffer);
}

bool Win32Platform::SetWorkingDirectory(const String& path)
{
    return !SetCurrentDirectoryW(*path);
}

void Win32Platform::FreeLibrary(void* handle)
{
    ::FreeLibrary((HMODULE)handle);
}

void* Win32Platform::GetProcAddress(void* handle, const char* symbol)
{
    return (void*)::GetProcAddress((HMODULE)handle, symbol);
}

#endif
