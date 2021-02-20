// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_WIN32

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/BatteryInfo.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Math/Math.h"
#include "IncludeWindowsHeaders.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include <Psapi.h>
#include <WinSock2.h>
#include <IPHlpApi.h>
#include <oleauto.h>
#include <WinBase.h>
#include <xmmintrin.h>
#pragma comment(lib, "Iphlpapi.lib")

namespace
{
    Guid DeviceId;
    CPUInfo CpuInfo;
    uint64 ClockFrequency;
    double CyclesToSeconds;
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

        if (FALSE == rc)
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

            // A hyper threaded core supplies more than one logical processor
            logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
            break;

        case RelationCache:
            // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache.
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
            // Logical processors share a physical package
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

    // Get page size
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    CpuInfo.PageSize = siSysInfo.dwPageSize;

    // Get clock speed
    CpuInfo.ClockSpeed = GetClockFrequency();

    // Get cache line size
    {
        int args[4];
        __cpuid(args, 0x80000006);
        CpuInfo.CacheLineSize = args[2] & 0xFF;
        ASSERT(CpuInfo.CacheLineSize && Math::IsPowerOfTwo(CpuInfo.CacheLineSize));
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

    return false;
}

void Win32Platform::MemoryBarrier()
{
    // NOTE: _ReadWriteBarrier and friends only prevent the
    // compiler from reordering loads and stores. To prevent
    // the CPU from doing the same, we have to use the
    // MemoryBarrier macro which expands to e.g. a serializing
    // XCHG instruction on x86. Also note that the MemoryBarrier
    // macro does *not* imply _ReadWriteBarrier, so that call
    // cannot be eliminated.

    _ReadWriteBarrier();

    // MemoryBarrier macro (we use undef to hide some symbols from Windows headers)
#if PLATFORM_64BITS

#ifdef _AMD64_
    __faststorefence();
#elif defined(_IA64_)
	__mf();
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

int64 Win32Platform::InterlockedExchange(int64 volatile* dst, int64 exchange)
{
    return InterlockedExchange64(dst, exchange);
}

int32 Win32Platform::InterlockedCompareExchange(int32 volatile* dst, int32 exchange, int32 comperand)
{
    static_assert(sizeof(int32) == sizeof(LONG), "Invalid LONG size.");
    return _InterlockedCompareExchange((LONG volatile*)dst, exchange, comperand);
}

int64 Win32Platform::InterlockedCompareExchange(int64 volatile* dst, int64 exchange, int64 comperand)
{
    return InterlockedCompareExchange64(dst, exchange, comperand);
}

int64 Win32Platform::InterlockedIncrement(int64 volatile* dst)
{
    return InterlockedIncrement64(dst);
}

int64 Win32Platform::InterlockedDecrement(int64 volatile* dst)
{
    return InterlockedDecrement64(dst);
}

int64 Win32Platform::InterlockedAdd(int64 volatile* dst, int64 value)
{
    return InterlockedExchangeAdd64(dst, value);
}

int32 Win32Platform::AtomicRead(int32 volatile* dst)
{
    static_assert(sizeof(int32) == sizeof(LONG), "Invalid LONG size.");
    return _InterlockedCompareExchange((LONG volatile*)dst, 0, 0);
}

int64 Win32Platform::AtomicRead(int64 volatile* dst)
{
    return InterlockedCompareExchange64(dst, 0, 0);
}

void Win32Platform::AtomicStore(int32 volatile* dst, int32 value)
{
    static_assert(sizeof(int32) == sizeof(LONG), "Invalid LONG size.");
    _InterlockedExchange((LONG volatile*)dst, value);
}

void Win32Platform::AtomicStore(int64 volatile* dst, int64 value)
{
    InterlockedExchange64(dst, value);
}

void Win32Platform::Prefetch(void const* ptr)
{
    _mm_prefetch((char const*)ptr, _MM_HINT_T0);
}

void* Win32Platform::Allocate(uint64 size, uint64 alignment)
{
#if COMPILE_WITH_PROFILER
    TrackAllocation(size);
#endif
    return _aligned_malloc((size_t)size, (size_t)alignment);
}

void Win32Platform::Free(void* ptr)
{
    _aligned_free(ptr);
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

BatteryInfo Win32Platform::GetBatteryInfo()
{
    BatteryInfo info;
    SYSTEM_POWER_STATUS status;
    GetSystemPowerStatus(&status);
    info.BatteryLifePercent = (float)status.BatteryLifePercent / 255.0f;
    if (status.BatteryFlag & 8)
        info.State = BatteryInfo::States::BatteryCharging;
    else if (status.BatteryFlag & 1 || status.BatteryFlag & 2 || status.BatteryFlag & 4)
        info.State = BatteryInfo::States::BatteryDischarging;
    else if (status.ACLineStatus == 1 || status.BatteryFlag & 128)
        info.State = BatteryInfo::States::Connected;
    return info;
}

CPUInfo Win32Platform::GetCPUInfo()
{
    return CpuInfo;
}

int32 Win32Platform::GetCacheLineSize()
{
    return CpuInfo.CacheLineSize;
}

MemoryStats Win32Platform::GetMemoryStats()
{
    // Get memory stats
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);

    // Fill result data
    MemoryStats result;
    result.TotalPhysicalMemory = statex.ullTotalPhys;
    result.UsedPhysicalMemory = statex.ullTotalPhys - statex.ullAvailPhys;
    result.TotalVirtualMemory = statex.ullTotalVirtual;
    result.UsedVirtualMemory = statex.ullTotalVirtual - statex.ullAvailVirtual;

    return result;
}

ProcessMemoryStats Win32Platform::GetProcessMemoryStats()
{
    // Get memory stats
    PROCESS_MEMORY_COUNTERS_EX countersEx;
    countersEx.cb = sizeof(countersEx);
    GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&countersEx, sizeof(countersEx));

    // Fill result data
    ProcessMemoryStats result;
    result.UsedPhysicalMemory = countersEx.WorkingSetSize;
    result.UsedVirtualMemory = countersEx.PrivateUsage;

    return result;
}

uint64 Win32Platform::GetCurrentProcessId()
{
    return ::GetCurrentProcessId();
}

uint64 Win32Platform::GetCurrentThreadID()
{
    return ::GetCurrentThreadId();
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
    ::Sleep(static_cast<DWORD>(milliseconds));
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
