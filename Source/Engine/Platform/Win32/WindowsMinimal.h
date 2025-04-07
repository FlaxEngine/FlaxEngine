// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#define WIN_API extern "C" __declspec(dllimport)
#define WIN_API_CALLCONV __stdcall

struct HINSTANCE__;
struct HWND__;
struct HKEY__;
struct HDC__;
struct HICON__;

namespace Windows
{
    typedef signed int BOOL;
    typedef unsigned long DWORD;
    typedef DWORD* LPDWORD;
    typedef int INT;
    typedef unsigned int UINT;
    typedef long LONG;
    typedef unsigned long ULONG;
    typedef long* LPLONG;
    typedef long long LONGLONG;
    typedef LONGLONG* LPLONGLONG;
    typedef void* LPVOID;
    typedef const void* LPCVOID;
    typedef const wchar_t* LPCTSTR;
#if defined(WIN64)
    typedef __int64 INT_PTR, *PINT_PTR;
    typedef unsigned __int64 UINT_PTR, *PUINT_PTR;
    typedef __int64 LONG_PTR, *PLONG_PTR;
    typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;
#else
    typedef int INT_PTR, *PINT_PTR;
    typedef unsigned int UINT_PTR, *PUINT_PTR;
    typedef long LONG_PTR, *PLONG_PTR;
    typedef unsigned long ULONG_PTR, *PULONG_PTR;
#endif
    typedef UINT_PTR WPARAM;
    typedef LONG_PTR LPARAM;
    typedef LONG_PTR LRESULT;
    typedef long HRESULT;

    typedef void* HANDLE;
    typedef HINSTANCE__* HINSTANCE;
    typedef HINSTANCE HMODULE;
    typedef HWND__* HWND;
    typedef HKEY__* HKEY;
    typedef HDC__* HDC;
    typedef HICON__* HICON;
    typedef HICON__* HCURSOR;

    struct CRITICAL_SECTION
    {
        void* Data1[1];
        long Data2[2];
        void* Data3[3];
    };

    struct CONDITION_VARIABLE
    {
        void* Ptr;
    };

    struct OVERLAPPED
    {
        void* Data1[3];
        unsigned long Data2[2];
    };

    struct GUID
    {
        unsigned long Data1;
        unsigned short Data2;
        unsigned short Data3;
        unsigned char Data4[8];
    };

    struct POINTL
    {
        LONG x;
        LONG y;
    };

    WIN_API void WIN_API_CALLCONV InitializeCriticalSectionEx(CRITICAL_SECTION* lpCriticalSection, DWORD dwSpinCount, DWORD Flags);
    WIN_API BOOL WIN_API_CALLCONV TryEnterCriticalSection(CRITICAL_SECTION* lpCriticalSection);
    WIN_API void WIN_API_CALLCONV EnterCriticalSection(CRITICAL_SECTION* lpCriticalSection);
    WIN_API void WIN_API_CALLCONV LeaveCriticalSection(CRITICAL_SECTION* lpCriticalSection);
    WIN_API void WIN_API_CALLCONV DeleteCriticalSection(CRITICAL_SECTION* lpCriticalSection);

    WIN_API void WIN_API_CALLCONV InitializeConditionVariable(CONDITION_VARIABLE* ConditionVariable);
    WIN_API BOOL WIN_API_CALLCONV SleepConditionVariableCS(CONDITION_VARIABLE* ConditionVariable, CRITICAL_SECTION* CriticalSection, DWORD dwMilliseconds);
    WIN_API void WIN_API_CALLCONV WakeConditionVariable(CONDITION_VARIABLE* ConditionVariable);
    WIN_API void WIN_API_CALLCONV WakeAllConditionVariable(CONDITION_VARIABLE* ConditionVariable);

    class IDataObject;
    typedef GUID IID;

    class IUnknown
    {
    public:
        virtual HRESULT WIN_API_CALLCONV QueryInterface(const IID& riid, void** ppvObject) = 0;
        virtual ULONG WIN_API_CALLCONV AddRef() = 0;
        virtual ULONG WIN_API_CALLCONV Release() = 0;

        template<class Q>
        HRESULT WIN_API_CALLCONV QueryInterface(Q** pp)
        {
            return QueryInterface(__uuidof(Q), (void**)pp);
        }
    };

    class IDropTarget : public IUnknown
    {
    public:
        virtual HRESULT WIN_API_CALLCONV DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) = 0;
        virtual HRESULT WIN_API_CALLCONV DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) = 0;
        virtual HRESULT WIN_API_CALLCONV DragLeave() = 0;
        virtual HRESULT WIN_API_CALLCONV Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) = 0;
    };
}

#endif
