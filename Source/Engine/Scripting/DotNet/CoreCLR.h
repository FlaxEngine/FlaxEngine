#pragma once

// FIXME
#include <ThirdParty/mono-2.0/mono/metadata/blob.h>

#include "Engine/Core/Types/String.h"
#include "Engine/Scripting/Types.h"

#if defined(_WIN32)
#define CORECLR_DELEGATE_CALLTYPE __stdcall
#define FLAX_CORECLR_STRING String
#else
#define CORECLR_DELEGATE_CALLTYPE
#define FLAX_CORECLR_STRING StringAnsi
#endif

class CoreCLR
{
private:
public:
    static bool LoadHostfxr(const String& library_path);
    static bool InitHostfxr(const String& config_path, const String& library_path);

    static void* GetFunctionPointerFromDelegate(const String& methodName);
    static void* GetStaticMethodPointer(const String& methodName);

    template<typename RetType, typename ...Args>
    static RetType CallStaticMethodInternal(const String& methodName, Args... args)
    {
        typedef RetType(CORECLR_DELEGATE_CALLTYPE* fun)(Args...);
        fun function = (fun)GetStaticMethodPointer(methodName);
        return function(args...);
    }

    template<typename RetType, typename ...Args>
    static RetType CallStaticMethodInternalPointer(void* funPtr, Args... args)
    {
        typedef RetType(CORECLR_DELEGATE_CALLTYPE* fun)(Args...);
        fun function = (fun)funPtr;
        return function(args...);
    }

    static const char* GetClassFullname(void* klass);
    static void* Allocate(int size);
    static void Free(void* ptr);
    static gchandle NewGCHandle(void* obj, bool pinned);
    static gchandle NewGCHandleWeakref(void* obj, bool track_resurrection);
    static void* GetGCHandleTarget(const gchandle& gchandle);
    static void FreeGCHandle(const gchandle& gchandle);
};
