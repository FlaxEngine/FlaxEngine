// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WINDOWS

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/Window.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/CreateWindowSettings.h"
#include "Engine/Platform/CreateProcessSettings.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Platform/BatteryInfo.h"
#include "Engine/Platform/Base/PlatformUtils.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Version.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "../Win32/IncludeWindowsHeaders.h"
#include <VersionHelpers.h>
#include <ShellAPI.h>
#include <Psapi.h>
#include <objbase.h>
#include <cstdio>
#undef ShellExecute
#if CRASH_LOG_ENABLE
#include <dbghelp.h>
#endif
#include "resource.h"

#define CLR_EXCEPTION 0xE0434352
#define VCPP_EXCEPTION 0xE06D7363

const Char* WindowsPlatform::ApplicationWindowClass = TEXT("FlaxWindow");
void* WindowsPlatform::Instance = nullptr;

#if CRASH_LOG_ENABLE || TRACY_ENABLE
// Lock for symbols list, shared with Tracy
extern "C" {
static HANDLE dbgHelpLock;

void FlaxDbgHelpInit()
{
    dbgHelpLock = CreateMutexW(nullptr, FALSE, nullptr);
}

void FlaxDbgHelpLock()
{
    WaitForSingleObject(dbgHelpLock, INFINITE);
}

void FlaxDbgHelpUnlock()
{
    ReleaseMutex(dbgHelpLock);
}
}
#endif

namespace
{
    String UserLocale, ComputerName, WindowsName;
    HANDLE EngineMutex = nullptr;
    Rectangle VirtualScreenBounds(0.0f, 0.0f, 0.0f, 0.0f);
    int32 VersionMajor = 0;
    int32 VersionMinor = 0;
    int32 VersionBuild = 0;
    int32 SystemDpi = 96;
#if CRASH_LOG_ENABLE
#if TRACY_ENABLE
    bool SymInitialized = true;
#else
    bool SymInitialized = false;
#endif
    Array<String> SymbolsPath;

    void OnSymbolsPathModified()
    {
        if (!SymInitialized)
            return;
        HANDLE process = GetCurrentProcess();
        SymCleanup(process);
        String symbolSearchPath;
        for (auto& path : SymbolsPath)
        {
            symbolSearchPath += path;
            symbolSearchPath += ";";
        }
        symbolSearchPath += Platform::GetWorkingDirectory();
        SymInitializeW(process, *symbolSearchPath, TRUE);
        //SymSetSearchPathW(process, *symbolSearchPath);
        //SymRefreshModuleList(process);
    }
#endif
}

HMONITOR GetPrimaryMonitorHandle()
{
    const POINT ptZero = { 0, 0 };
    return MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
}

int32 CalculateDpi(HMODULE shCoreDll)
{
    int32 dpiX = 96, dpiY = 96;

    if (shCoreDll)
    {
        typedef HRESULT (STDAPICALLTYPE * GetDPIForMonitorProc)(HMONITOR hmonitor, UINT dpiType, UINT* dpiX, UINT* dpiY);
        const GetDPIForMonitorProc getDPIForMonitor = (GetDPIForMonitorProc)GetProcAddress(shCoreDll, "GetDpiForMonitor");

        if (getDPIForMonitor)
        {
            HMONITOR monitor = GetPrimaryMonitorHandle();

            UINT x = 0, y = 0;
            HRESULT hr = getDPIForMonitor(monitor, 0, &x, &y);
            if (SUCCEEDED(hr) && (x > 0) && (y > 0))
            {
                dpiX = (int32)x;
                dpiY = (int32)y;
            }
        }

        FreeLibrary(shCoreDll);
    }

    return (dpiX + dpiY) / 2;
}

LONG GetStringRegKey(HKEY hKey, const Char* strValueName, String& strValue, const String& strDefaultValue)
{
    strValue = strDefaultValue;
    WCHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError;
    nError = RegQueryValueExW(hKey, strValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(szBuffer), &dwBufferSize);
    if (ERROR_SUCCESS == nError)
    {
        strValue = szBuffer;
    }
    return nError;
}

LONG GetDWORDRegKey(HKEY hKey, const Char* strValueName, DWORD& nValue, DWORD nDefaultValue)
{
    nValue = nDefaultValue;
    DWORD dwBufferSize(sizeof(DWORD));
    DWORD nResult(0);
    LONG nError = RegQueryValueExW(hKey, strValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(&nResult), &dwBufferSize);
    if (ERROR_SUCCESS == nError)
    {
        nValue = nResult;
    }
    return nError;
}

void GetWindowsVersion(String& windowsName, int32& versionMajor, int32& versionMinor, int32& versionBuild)
{
    // Get OS version

    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &hKey);
    if (lRes == ERROR_SUCCESS)
    {
        GetStringRegKey(hKey, TEXT("ProductName"), windowsName, TEXT("Windows"));

        DWORD currentMajorVersionNumber;
        DWORD currentMinorVersionNumber;
        String currentBuildNumber;
        GetDWORDRegKey(hKey, TEXT("CurrentMajorVersionNumber"), currentMajorVersionNumber, 0);
        GetDWORDRegKey(hKey, TEXT("CurrentMinorVersionNumber"), currentMinorVersionNumber, 0);
        GetStringRegKey(hKey, TEXT("CurrentBuildNumber"), currentBuildNumber, TEXT("0"));
        VersionMajor = currentMajorVersionNumber;
        VersionMinor = currentMinorVersionNumber;
        StringUtils::Parse(currentBuildNumber.Get(), &VersionBuild);

        if (StringUtils::Compare(windowsName.Get(), TEXT("Windows 7"), 9) == 0)
        {
            VersionMajor = 6;
            VersionMinor = 2;
        }
        else if (VersionMajor >= 10 && VersionBuild >= 22000)
        {
            // Windows 11
            windowsName.Replace(TEXT("10"), TEXT("11"));
        }
        else if (VersionMajor == 0 && VersionMinor == 0)
        {
            String windowsVersion;
            GetStringRegKey(hKey, TEXT("CurrentVersion"), windowsVersion, TEXT(""));

            if (windowsVersion.HasChars())
            {
                const int32 dot = windowsVersion.Find('.');
                if (dot != -1)
                {
                    StringUtils::Parse(windowsVersion.Substring(0, dot).Get(), &VersionMajor);
                    StringUtils::Parse(windowsVersion.Substring(dot + 1).Get(), &VersionMinor);
                }
            }
        }
    }
    else
    {
        if (IsWindowsServer())
        {
            windowsName = TEXT("Windows Server");
            versionMajor = 6;
            versionMinor = 3;
        }
        else if (IsWindows8Point1OrGreater())
        {
            windowsName = TEXT("Windows 8.1");
            versionMajor = 6;
            versionMinor = 3;
        }
        else if (IsWindows8OrGreater())
        {
            windowsName = TEXT("Windows 8");
            versionMajor = 6;
            versionMinor = 2;
        }
        else if (IsWindows7SP1OrGreater())
        {
            windowsName = TEXT("Windows 7 SP1");
            versionMajor = 6;
            versionMinor = 2;
        }
        else if (IsWindows7OrGreater())
        {
            windowsName = TEXT("Windows 7");
            versionMajor = 6;
            versionMinor = 1;
        }
        else if (IsWindowsVistaSP2OrGreater())
        {
            windowsName = TEXT("Windows Vista SP2");
            versionMajor = 6;
            versionMinor = 1;
        }
        else if (IsWindowsVistaSP1OrGreater())
        {
            windowsName = TEXT("Windows Vista SP1");
            versionMajor = 6;
            versionMinor = 1;
        }
        else if (IsWindowsVistaOrGreater())
        {
            windowsName = TEXT("Windows Vista");
            versionMajor = 6;
            versionMinor = 0;
        }
    }
    RegCloseKey(hKey);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Find window to process that message
    if (hwnd != nullptr)
    {
        // Find window by handle
        const auto win = WindowsManager::GetByNativePtr(hwnd);
        if (win)
        {
            return static_cast<WindowsWindow*>(win)->WndProc(msg, wParam, lParam);
        }
    }

    // Default
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

long __stdcall WindowsPlatform::SehExceptionHandler(EXCEPTION_POINTERS* ep)
{
    if (ep->ExceptionRecord->ExceptionCode == CLR_EXCEPTION)
    {
        // Pass CLR exceptions back to runtime
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // Skip if engine already crashed
    if (Engine::FatalError != FatalErrorType::None)
        return EXCEPTION_CONTINUE_SEARCH;

    // Get exception info
    String errorMsg(TEXT("Unhandled exception: "));
    switch (ep->ExceptionRecord->ExceptionCode)
    {
#define CASE(x) case x: errorMsg += TEXT(#x); break;
    CASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
    CASE(EXCEPTION_DATATYPE_MISALIGNMENT)
    CASE(EXCEPTION_FLT_DENORMAL_OPERAND)
    CASE(EXCEPTION_FLT_DIVIDE_BY_ZERO)
    CASE(EXCEPTION_FLT_INVALID_OPERATION)
    CASE(EXCEPTION_ILLEGAL_INSTRUCTION)
    CASE(EXCEPTION_INT_DIVIDE_BY_ZERO)
    CASE(EXCEPTION_PRIV_INSTRUCTION)
    CASE(EXCEPTION_STACK_OVERFLOW)
#undef CASE
    case EXCEPTION_ACCESS_VIOLATION:
        errorMsg += TEXT("EXCEPTION_ACCESS_VIOLATION ");
        if (ep->ExceptionRecord->ExceptionInformation[0] == 0)
        {
            errorMsg += TEXT("reading address ");
        }
        else if (ep->ExceptionRecord->ExceptionInformation[0] == 1)
        {
            errorMsg += TEXT("writing address ");
        }
        errorMsg += String::Format(TEXT("{:#x}"), (uint32)ep->ExceptionRecord->ExceptionInformation[1]);
        break;
    default:
        errorMsg += String::Format(TEXT("{:#x}"), (uint32)ep->ExceptionRecord->ExceptionCode);
    }

    // Log exception and return to the crash location when using debugger
    if (Platform::IsDebuggerPresent())
    {
        LOG_STR(Error, errorMsg);
        const String stackTrace = Platform::GetStackTrace(0, 60, ep);
        LOG_STR(Error, stackTrace);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // Crash engine
    Platform::Fatal(errorMsg.Get(), ep, FatalErrorType::Exception);
    return EXCEPTION_CONTINUE_SEARCH;
}

#if CRASH_LOG_ENABLE

bool GetModuleListPSAPI(HANDLE hProcess)
{
    typedef BOOL (WINAPI* EnumProcessModulesFunc)(HANDLE hProcess, HMODULE* lphModule, DWORD cb, LPDWORD lpcbNeeded);
    typedef DWORD (WINAPI* GetModuleFileNameExFunc)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize);
    typedef DWORD (WINAPI* GetModuleBaseNameFunc)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize);
    typedef BOOL (WINAPI* GetModuleInformationFunc)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO pmi, DWORD nSize);

    MODULEINFO mi;
    const SIZE_T bufferSize = 8096;

    HINSTANCE psapiDll = LoadLibraryW(TEXT("psapi.dll"));
    if (psapiDll == nullptr)
        return true;

    auto EnumProcessModulesFuncPtr = (EnumProcessModulesFunc)GetProcAddress(psapiDll, "EnumProcessModules");
    auto GetModuleFileNameExFuncPtr = (GetModuleFileNameExFunc)GetProcAddress(psapiDll, "GetModuleFileNameExA");
    auto GetModuleBaseNameFuncPtr = (GetModuleFileNameExFunc)GetProcAddress(psapiDll, "GetModuleBaseNameA");
    auto GetModuleInformationFuncPtr = (GetModuleInformationFunc)GetProcAddress(psapiDll, "GetModuleInformation");
    if ((EnumProcessModulesFuncPtr == nullptr) || (GetModuleFileNameExFuncPtr == nullptr) || (GetModuleBaseNameFuncPtr == nullptr) || (GetModuleInformationFuncPtr == nullptr))
    {
        FreeLibrary(psapiDll);
        return true;
    }

    auto hMods = (HMODULE*)malloc(sizeof(HMODULE) * (bufferSize / sizeof(HMODULE)));
    auto tt = (char*)malloc(sizeof(char) * bufferSize);
    auto tt2 = (char*)malloc(sizeof(char) * bufferSize);
    if ((hMods == nullptr) || (tt == nullptr) || (tt2 == nullptr))
        goto cleanup;
    DWORD cbNeeded;
    if (!EnumProcessModulesFuncPtr(hProcess, hMods, bufferSize, &cbNeeded))
        goto cleanup;
    if (cbNeeded > bufferSize)
        goto cleanup;

    for (DWORD i = 0; i < cbNeeded / sizeof(hMods[0]); i++)
    {
        // Base address, Size
        GetModuleInformationFuncPtr(hProcess, hMods[i], &mi, sizeof(mi));
        // Image file name
        tt[0] = 0;
        GetModuleFileNameExFuncPtr(hProcess, hMods[i], tt, bufferSize);
        // Module name
        tt2[0] = 0;
        GetModuleBaseNameFuncPtr(hProcess, hMods[i], tt2, bufferSize);

        SymLoadModule64(hProcess, 0, tt, tt2, (DWORD64)mi.lpBaseOfDll, mi.SizeOfImage);
    }

cleanup:
    if (psapiDll != nullptr)
        FreeLibrary(psapiDll);
    if (tt2 != nullptr)
        free(tt2);
    if (tt != nullptr)
        free(tt);
    if (hMods != nullptr)
        free(hMods);
    return false;
}

#endif

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    // Construct input flags
    UINT flags = 0;
    switch (buttons)
    {
    case MessageBoxButtons::AbortRetryIgnore:
        flags |= MB_ABORTRETRYIGNORE;
        break;
    case MessageBoxButtons::OK:
        flags |= MB_OK;
        break;
    case MessageBoxButtons::OKCancel:
        flags |= MB_OKCANCEL;
        break;
    case MessageBoxButtons::RetryCancel:
        flags |= MB_RETRYCANCEL;
        break;
    case MessageBoxButtons::YesNo:
        flags |= MB_YESNO;
        break;
    case MessageBoxButtons::YesNoCancel:
        flags |= MB_YESNOCANCEL;
        break;
    default:
        break;
    }
    switch (icon)
    {
    case MessageBoxIcon::Asterisk:
        flags |= MB_ICONASTERISK;
        break;
    case MessageBoxIcon::Error:
        flags |= MB_ICONERROR;
        break;
    case MessageBoxIcon::Exclamation:
        flags |= MB_ICONEXCLAMATION;
        break;
    case MessageBoxIcon::Hand:
        flags |= MB_ICONHAND;
        break;
    case MessageBoxIcon::Information:
        flags |= MB_ICONINFORMATION;
        break;
    case MessageBoxIcon::Question:
        flags |= MB_ICONQUESTION;
        break;
    case MessageBoxIcon::Stop:
        flags |= MB_ICONSTOP;
        break;
    case MessageBoxIcon::Warning:
        flags |= MB_ICONWARNING;
        break;
    default:
        break;
    }
    flags |= MB_TASKMODAL;

    // Show dialog
    int result = MessageBoxW(parent ? static_cast<HWND>(parent->GetNativePtr()) : nullptr, String(text).GetText(), String(caption).GetText(), flags);

    // Translate result to dialog result
    DialogResult dialogResult;
    switch (result)
    {
    case IDABORT:
        dialogResult = DialogResult::Abort;
        break;
    case IDCANCEL:
        dialogResult = DialogResult::Cancel;
        break;
    case IDCONTINUE:
        dialogResult = DialogResult::OK;
        break;
    case IDIGNORE:
        dialogResult = DialogResult::Ignore;
        break;
    case IDNO:
        dialogResult = DialogResult::No;
        break;
    case IDOK:
        dialogResult = DialogResult::OK;
        break;
    case IDRETRY:
        dialogResult = DialogResult::Retry;
        break;
    case IDYES:
        dialogResult = DialogResult::Yes;
        break;
    default:
        dialogResult = DialogResult::None;
        break;
    }
    return dialogResult;
}

bool WindowsPlatform::CreateMutex(const Char* name)
{
    EngineMutex = CreateMutexW(nullptr, true, name);
    if (EngineMutex && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return false;
    }

    return true;
}

void WindowsPlatform::ReleaseMutex()
{
    if (EngineMutex)
    {
        ::ReleaseMutex(EngineMutex);
        EngineMutex = nullptr;
    }
}

void WindowsPlatform::PreInit(void* hInstance)
{
    ASSERT(hInstance);
    Instance = hInstance;

    // Disable the process from being showing "ghosted" while not responding messages during slow tasks
    DisableProcessWindowsGhosting();

    // Register window class
    WNDCLASS windowsClass;
    Platform::MemoryClear(&windowsClass, sizeof(WNDCLASS));
    windowsClass.style = CS_DBLCLKS;
    windowsClass.lpfnWndProc = WndProc;
    windowsClass.hInstance = (HINSTANCE)Instance;
    windowsClass.hIcon = LoadIconW(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_MAINFRAME));
    windowsClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowsClass.lpszClassName = ApplicationWindowClass;
    if (!RegisterClassW(&windowsClass))
    {
        Error(TEXT("Window class registration failed!"));
        exit(-1);
    }

    // Init OLE
    if (OleInitialize(nullptr) != S_OK)
    {
        Error(TEXT("OLE initalization failed!"));
        exit(-1);
    }

#if CRASH_LOG_ENABLE
    TCHAR buffer[MAX_PATH] = { 0 };
    FlaxDbgHelpLock();
    if (::GetModuleFileNameW(::GetModuleHandleW(nullptr), buffer, MAX_PATH))
        SymbolsPath.Add(StringUtils::GetDirectoryName(buffer));
    if (::GetEnvironmentVariableW(TEXT("_NT_SYMBOL_PATH"), buffer, MAX_PATH))
        SymbolsPath.Add(StringUtils::GetDirectoryName(buffer));
    DWORD options = SymGetOptions();
    options |= SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_DEFERRED_LOADS | SYMOPT_EXACT_SYMBOLS;
    SymSetOptions(options);
    OnSymbolsPathModified();
    FlaxDbgHelpUnlock();
#endif

    GetWindowsVersion(WindowsName, VersionMajor, VersionMinor, VersionBuild);

    // Validate platform
    if (VersionMajor < 6)
    {
        Error(TEXT("Not supported operating system version."));
        exit(-1);
    }
}

bool WindowsPlatform::IsWindows10()
{
    return VersionMajor >= 10;
}

bool WindowsPlatform::ReadRegValue(void* root, const String& key, const String& name, String* result)
{
    HKEY hKey;
    if (RegOpenKeyExW((HKEY)root, *key, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        // "Could not open registry key";
        return true;
    }

    DWORD type;
    DWORD cbData;
    if (RegQueryValueExW(hKey, *name, nullptr, &type, nullptr, &cbData) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        // "Could not read registry value";
        return true;
    }

    if (type != REG_SZ)
    {
        RegCloseKey(hKey);
        // "Incorrect registry value type";
        return true;
    }

    Array<Char> data;
    data.Resize((int32)cbData / sizeof(Char));
    if (RegQueryValueExW(hKey, *name, nullptr, nullptr, reinterpret_cast<LPBYTE>(data.Get()), &cbData) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        // "Could not read registry value";
        return true;
    }

    RegCloseKey(hKey);

    *result = data.Get();
    return false;
}

bool WindowsPlatform::Init()
{
    if (Win32Platform::Init())
        return true;

    // Init console output (engine is linked with /SUBSYSTEM:WINDOWS so it lacks of proper console output on Windows)
    if (CommandLine::Options.Std.IsTrue())
    {
        // Attaches output of application to parent console, returns true if running in console-mode
        // [Reference: https://www.tillett.info/2013/05/13/how-to-create-a-windows-program-that-works-as-both-as-a-gui-and-console-application]
        if (AttachConsole(ATTACH_PARENT_PROCESS))
        {
            const HANDLE consoleHandleOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (consoleHandleOut != INVALID_HANDLE_VALUE)
            {
                freopen("CONOUT$", "w", stdout);
                setvbuf(stdout, NULL, _IONBF, 0);
            }
            const HANDLE consoleHandleError = GetStdHandle(STD_ERROR_HANDLE);
            if (consoleHandleError != INVALID_HANDLE_VALUE)
            {
                freopen("CONOUT$", "w", stderr);
                setvbuf(stderr, NULL, _IONBF, 0);
            }
        }
    }

    // Check if can run Engine on current platform
#if WINVER >= 0x0A00
    if (!IsWindows10OrGreater() && !IsWindowsServer())
    {
        Platform::Fatal(TEXT("Flax Engine requires Windows 10 or higher."));
        return true;
    }
#elif WINVER >= 0x0603
    if (!IsWindows8Point1OrGreater() && !IsWindowsServer())
    {
        Platform::Fatal(TEXT("Flax Engine requires Windows 8.1 or higher."));
        return true;
    }
#elif WINVER >= 0x0602
    if (!IsWindows8OrGreater() && !IsWindowsServer())
    {
        Platform::Fatal(TEXT("Flax Engine requires Windows 8 or higher."));
        return true;
    }
#else
    if (!IsWindows7OrGreater() && !IsWindowsServer())
    {
        Platform::Fatal(TEXT("Flax Engine requires Windows 7 or higher."));
        return true;
    }
#endif

    // Set the lowest possible timer resolution
    const HMODULE ntdll = LoadLibraryW(L"ntdll.dll");
    if (ntdll)
    {
        typedef LONG (WIN_API_CALLCONV *NtSetTimerResolution)(ULONG DesiredResolution, unsigned char SetResolution, ULONG* CurrentResolution);
        const NtSetTimerResolution ntSetTimerResolution = (NtSetTimerResolution)GetProcAddress(ntdll, "NtSetTimerResolution");
        if (ntSetTimerResolution)
        {
            ULONG currentResolution;
            ntSetTimerResolution(1, TRUE, &currentResolution);
        }
        ::FreeLibrary(ntdll);
    }

    DWORD tmp;
    Char buffer[256];

    // Get user locale string
    if (GetUserDefaultLocaleName(buffer, LOCALE_NAME_MAX_LENGTH))
    {
        UserLocale = String(buffer);
    }

    // Get computer name string
    if (GetComputerNameW(buffer, &tmp))
    {
        ComputerName = String(buffer);
    }

    // Get user name string
    String userName;
    if (GetUserNameW(buffer, &tmp))
    {
        userName = String(buffer);
    }
    OnPlatformUserAdd(New<User>(userName));

    WindowsInput::Init();

    return false;
}

void WindowsPlatform::LogInfo()
{
    Win32Platform::LogInfo();

#if PLATFORM_ARCH_X86 || PLATFORM_ARCH_X64
    // Log CPU brand
    {
	    char brandBuffer[0x40] = {};
	    int32 cpuInfo[4] = { -1 };
	    __cpuid(cpuInfo, 0x80000000);
	    if (cpuInfo[0] >= 0x80000004)
	    {
		    for (uint32 i = 0; i < 3; i++)
		    {
			    __cpuid(cpuInfo, 0x80000002 + i);
			    memcpy(brandBuffer + i * sizeof(cpuInfo), cpuInfo, sizeof(cpuInfo));
		    }
	    }
        LOG(Info, "CPU: {0}", String(brandBuffer));
    }
#endif

    LOG(Info, "Microsoft {0} {1}-bit ({2}.{3}.{4})", WindowsName, Platform::Is64BitPlatform() ? TEXT("64") : TEXT("32"), VersionMajor, VersionMinor, VersionBuild);

    // Check minimum amount of RAM
    auto memStats = Platform::GetMemoryStats();
    uint64 mb = memStats.TotalPhysicalMemory / (1024 * 1024);
    uint64 mbMinimum = 2048;
    if (mb < mbMinimum * 0.8f)
    {
        auto msg = String::Format(TEXT("Not enough RAM memory for good application performance.\nDetected: {0} MB\nRecommended : {1} MB\nDo you want to continue ?"), mb, mbMinimum);
        if (MessageBoxW(nullptr, *msg, TEXT("Warning"), MB_ICONWARNING | MB_YESNO) == IDNO)
        {
            LOG(Warning, "Not enough RAM. Closing...");
            exit(0);
        }
    }
}

void WindowsPlatform::Tick()
{
    WindowsInput::Update();

    // Check to see if any messages are waiting in the queue
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        // Translate the message and dispatch it to WindowProc()
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void WindowsPlatform::BeforeExit()
{
}

void WindowsPlatform::Exit()
{
#if CRASH_LOG_ENABLE
    FlaxDbgHelpLock();
#if !TRACY_ENABLE
    if (SymInitialized)
    {
        SymInitialized = false;
        SymCleanup(GetCurrentProcess());
    }
#endif
    SymbolsPath.Resize(0);
    FlaxDbgHelpUnlock();
#endif

    // Unregister app class
    UnregisterClassW(ApplicationWindowClass, nullptr);

    Win32Platform::Exit();
}

#if !BUILD_RELEASE

void WindowsPlatform::Log(const StringView& msg)
{
    Char buffer[512];
    Char* str;
    if (msg.Length() + 3 < ARRAY_COUNT(buffer))
        str = buffer;
    else
        str = (Char*)Allocate((msg.Length() + 3) * sizeof(Char), 16);
    MemoryCopy(str, msg.Get(), msg.Length() * sizeof(Char));
    str[msg.Length() + 0] = '\r';
    str[msg.Length() + 1] = '\n';
    str[msg.Length() + 2] = 0;
    OutputDebugStringW(str);
    if (str != buffer)
        Free(str);
}

bool WindowsPlatform::IsDebuggerPresent()
{
    return !!::IsDebuggerPresent();
}

#endif

void WindowsPlatform::SetHighDpiAwarenessEnabled(bool enable)
{
    const HMODULE shCoreDll = LoadLibraryW(L"Shcore.dll");
    if (!shCoreDll)
        return;
    typedef enum _PROCESS_DPI_AWARENESS
    {
        PROCESS_DPI_UNAWARE = 0,
        PROCESS_SYSTEM_DPI_AWARE = 1,
        PROCESS_PER_MONITOR_DPI_AWARE = 2
    } PROCESS_DPI_AWARENESS;
    typedef HRESULT (STDAPICALLTYPE *SetProcessDpiAwarenessProc)(PROCESS_DPI_AWARENESS Value);
    const SetProcessDpiAwarenessProc setProcessDpiAwareness = (SetProcessDpiAwarenessProc)GetProcAddress(shCoreDll, "SetProcessDpiAwareness");
    if (setProcessDpiAwareness)
    {
        setProcessDpiAwareness(enable ? PROCESS_PER_MONITOR_DPI_AWARE : PROCESS_DPI_UNAWARE);
    }
    SystemDpi = CalculateDpi(shCoreDll);
    ::FreeLibrary(shCoreDll);
}

String WindowsPlatform::GetSystemName()
{
    return WindowsName;
}

Version WindowsPlatform::GetSystemVersion()
{
    return Version(VersionMajor, VersionMinor, VersionBuild);
}

BatteryInfo WindowsPlatform::GetBatteryInfo()
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

int32 WindowsPlatform::GetDpi()
{
    return SystemDpi;
}

String WindowsPlatform::GetUserLocaleName()
{
    return UserLocale;
}

String WindowsPlatform::GetComputerName()
{
    return ComputerName;
}

bool WindowsPlatform::GetHasFocus()
{
    DWORD foregroundProcess;
    GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
    return foregroundProcess == ::GetCurrentProcessId();
}

bool WindowsPlatform::CanOpenUrl(const StringView& url)
{
    return true;
}

void WindowsPlatform::OpenUrl(const StringView& url)
{
    ::ShellExecuteW(nullptr, TEXT("open"), *url, nullptr, nullptr, SW_SHOWNORMAL);
}

Float2 WindowsPlatform::GetMousePosition()
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    return Float2((float)cursorPos.x, (float)cursorPos.y);
}

void WindowsPlatform::SetMousePosition(const Float2& pos)
{
    ::SetCursorPos((int)pos.X, (int)pos.Y);
}

struct GetMonitorBoundsData
{
    Float2 Pos;
    Rectangle Result;

    GetMonitorBoundsData(const Float2& pos)
        : Pos(pos)
        , Result(Float2::Zero, WindowsPlatform::GetDesktopSize())
    {
    }
};

BOOL CALLBACK EnumMonitorSize(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    GetMonitorBoundsData* data = reinterpret_cast<GetMonitorBoundsData*>(dwData);
    Rectangle monitorRect(
        (float)lprcMonitor->left,
        (float)lprcMonitor->top,
        (float)(lprcMonitor->right - lprcMonitor->left),
        (float)(lprcMonitor->bottom - lprcMonitor->top));
    if (monitorRect.Contains(data->Pos))
    {
        data->Result = monitorRect;
        return FALSE;
    }
    return TRUE;
}

Rectangle WindowsPlatform::GetMonitorBounds(const Float2& screenPos)
{
    GetMonitorBoundsData data(screenPos);
    EnumDisplayMonitors(nullptr, nullptr, EnumMonitorSize, reinterpret_cast<LPARAM>(&data));
    return data.Result;
}

Float2 WindowsPlatform::GetDesktopSize()
{
    return Float2(
        static_cast<float>(GetSystemMetrics(SM_CXSCREEN)),
        static_cast<float>(GetSystemMetrics(SM_CYSCREEN))
    );
}

BOOL CALLBACK EnumMonitorTotalBounds(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    LPRECT l = reinterpret_cast<LPRECT>(dwData);
    UnionRect(l, l, lprcMonitor);
    return TRUE;
}

Rectangle WindowsPlatform::GetVirtualDesktopBounds()
{
    if (VirtualScreenBounds.Size.X == 0)
    {
        RECT screenRect = {};
        EnumDisplayMonitors(nullptr, nullptr, EnumMonitorTotalBounds, reinterpret_cast<LPARAM>(&screenRect));
        VirtualScreenBounds.Location.X = static_cast<float>(screenRect.left);
        VirtualScreenBounds.Location.Y = static_cast<float>(screenRect.top);
        VirtualScreenBounds.Size.X = static_cast<float>(screenRect.right - screenRect.left);
        VirtualScreenBounds.Size.Y = static_cast<float>(screenRect.bottom - screenRect.top);
    }
    return VirtualScreenBounds;
}

void WindowsPlatform::GetEnvironmentVariables(Dictionary<String, String>& result)
{
    const LPWCH environmentStr = GetEnvironmentStringsW();
    if (environmentStr)
    {
        LPWCH env = environmentStr;
        while (*env != '\0')
        {
            if (*env != '=')
            {
                WCHAR* str = wcschr(env, '=');
                ASSERT(str);
                result[String(env, (int32)(str - env))] = str + 1;
            }
            while (*env != '\0')
                env++;
            env++;
        }
        FreeEnvironmentStringsW(environmentStr);
    }
}

bool WindowsPlatform::GetEnvironmentVariable(const String& name, String& value)
{
    const int32 bufferSize = 512;
    Char buffer[bufferSize];
    DWORD result = GetEnvironmentVariableW(*name, buffer, bufferSize);
    if (result == 0)
    {
        //LOG_WIN32_LAST_ERROR;
        return true;
    }
    if (bufferSize < result)
    {
        value.ReserveSpace(result);
        result = GetEnvironmentVariableW(*name, *value, result);
        if (!result)
        {
            LOG_WIN32_LAST_ERROR;
            return FALSE;
        }
    }
    else
    {
        value.Set(buffer, result);
    }
    return false;
}

bool WindowsPlatform::SetEnvironmentVariable(const String& name, const String& value)
{
    if (!SetEnvironmentVariableW(*name, *value))
    {
        LOG_WIN32_LAST_ERROR;
        return true;
    }
    return false;
}

bool IsProcRunning(HANDLE handle)
{
    return WaitForSingleObject(handle, 0) == WAIT_TIMEOUT;
}

void ReadPipe(HANDLE pipe, Array<char>& rawData, Array<Char>& logData, LogType logType, CreateProcessSettings& settings)
{
    // Check if any data is ready to read
    DWORD bytesAvailable = 0;
    if (PeekNamedPipe(pipe, nullptr, 0, nullptr, &bytesAvailable, nullptr) && bytesAvailable > 0)
    {
        // Read data
        rawData.Clear();
        rawData.Resize(bytesAvailable);
        DWORD bytesRead = 0;
        if (ReadFile(pipe, rawData.Get(), bytesAvailable, &bytesRead, nullptr) && bytesRead > 0)
        {
            // Skip Windows-style lines
            rawData.RemoveAllKeepOrder('\r');

            // Remove last new line character
            if (rawData.Last() == '\n')
                rawData.RemoveLast();

            // Log contents
            logData.Clear();
            logData.Resize(rawData.Count() + 1);
            int32 tmp;
            StringUtils::ConvertANSI2UTF16(rawData.Get(), logData.Get(), rawData.Count(), tmp);
            logData.Last() = '\0';
            if (settings.LogOutput)
                Log::Logger::Write(logType, StringView(logData.Get(), rawData.Count()));
            if (settings.SaveOutput)
                settings.Output.Add(logData.Get(), rawData.Count());
        }
    }
}

int32 WindowsPlatform::CreateProcess(CreateProcessSettings& settings)
{
    LOG(Info, "Command: {0} {1}", settings.FileName, settings.Arguments);
    if (settings.WorkingDirectory.HasChars())
    {
        LOG(Info, "Working directory: {0}", settings.WorkingDirectory);
    }
    const bool captureStdOut = settings.LogOutput || settings.SaveOutput;

    int32 result = 0;
    if (settings.ShellExecute)
    {
        SHELLEXECUTEINFOW shExecInfo = { 0 };
        shExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
        shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExecInfo.lpFile = settings.FileName.GetText();
        shExecInfo.lpParameters = settings.Arguments.HasChars() ? settings.Arguments.Get() : nullptr;
        shExecInfo.lpDirectory = settings.WorkingDirectory.HasChars() ? settings.WorkingDirectory.Get() : nullptr;
        shExecInfo.nShow = settings.HiddenWindow ? SW_HIDE : SW_SHOW;
        if (ShellExecuteExW(&shExecInfo) == FALSE)
        {
            result = 1;
            LOG(Warning, "Cannot start process. Error code: 0x{0:x}", (uint64)GetLastError());
        }
        else if (settings.WaitForEnd)
        {
            WaitForSingleObject(shExecInfo.hProcess, INFINITE);
            DWORD exitCode;
            if (GetExitCodeProcess(shExecInfo.hProcess, &exitCode) != 0)
                result = exitCode;
            CloseHandle(shExecInfo.hProcess);
        }
    }
    else
    {
        result = -1;
        const String cmdLine = settings.FileName + TEXT(" ") + settings.Arguments;

        STARTUPINFOEX startupInfoEx;
        ZeroMemory(&startupInfoEx, sizeof(startupInfoEx));
        startupInfoEx.StartupInfo.cb = sizeof(startupInfoEx);
        if (settings.HiddenWindow)
        {
            startupInfoEx.StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
            startupInfoEx.StartupInfo.wShowWindow |= SW_HIDE | SW_SHOWNOACTIVATE;
        }

        DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | DETACHED_PROCESS;
        if (settings.HiddenWindow)
            dwCreationFlags |= CREATE_NO_WINDOW;

        Char* environmentStr = nullptr;
        if (settings.Environment.HasItems())
        {
            dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;

            int32 totalLength = 1;
            for (auto& e : settings.Environment)
                totalLength += e.Key.Length() + e.Value.Length() + 2;

            environmentStr = (Char*)Allocator::Allocate(totalLength * sizeof(Char));

            Char* env = environmentStr;
            for (auto& e : settings.Environment)
            {
                auto& key = e.Key;
                auto& value = e.Value;
                Platform::MemoryCopy(env, key.Get(), key.Length() * sizeof(Char));
                env += key.Length();
                *env++ = '=';
                Platform::MemoryCopy(env, value.Get(), value.Length() * sizeof(Char));
                env += value.Length();
                *env++ = 0;
            }
            *env++ = 0;
            ASSERT((uint64)env - (uint64)environmentStr == (uint64)(totalLength * sizeof(Char)));
        }

        HANDLE stdOutRead = nullptr;
        HANDLE stdErrRead = nullptr;

        if (captureStdOut)
        {
            dwCreationFlags |= EXTENDED_STARTUPINFO_PRESENT;
            startupInfoEx.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

            SECURITY_ATTRIBUTES securityAttributes;
            securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
            securityAttributes.bInheritHandle = TRUE;
            securityAttributes.lpSecurityDescriptor = nullptr;

            if (!CreatePipe(&stdOutRead, &startupInfoEx.StartupInfo.hStdOutput, &securityAttributes, 0) ||
                !CreatePipe(&stdErrRead, &startupInfoEx.StartupInfo.hStdError, &securityAttributes, 0))
            {
                LOG(Warning, "CreatePipe failed");
                return 1;
            }

            SIZE_T bufferSize = 0;
            if (!InitializeProcThreadAttributeList(nullptr, 1, 0, &bufferSize) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                startupInfoEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)Allocator::Allocate(bufferSize);
                if (!InitializeProcThreadAttributeList(startupInfoEx.lpAttributeList, 1, 0, &bufferSize))
                {
                    LOG(Warning, "InitializeProcThreadAttributeList failed");
                    goto ERROR_EXIT;
                }
            }

            HANDLE inheritHandles[2] = { startupInfoEx.StartupInfo.hStdOutput, startupInfoEx.StartupInfo.hStdError };
            if (!UpdateProcThreadAttribute(startupInfoEx.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inheritHandles, sizeof(inheritHandles), nullptr, nullptr))
            {
                LOG(Warning, "UpdateProcThreadAttribute failed");
                goto ERROR_EXIT;
            }
        }

        // Create the process
        PROCESS_INFORMATION procInfo;
        if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmdLine.GetText()), nullptr, nullptr, TRUE, dwCreationFlags, (LPVOID)environmentStr, settings.WorkingDirectory.HasChars() ? settings.WorkingDirectory.Get() : nullptr, &startupInfoEx.StartupInfo, &procInfo))
        {
            LOG(Warning, "Cannot start process. Error code: 0x{0:x}", (uint64)GetLastError());
            goto ERROR_EXIT;
        }

        if (stdOutRead != nullptr)
        {
            // Keep reading std output and std error streams until process is running
            Array<char> rawData;
            Array<Char> logData;
            do
            {
                ReadPipe(stdOutRead, rawData, logData, LogType::Info, settings);
                ReadPipe(stdErrRead, rawData, logData, LogType::Error, settings);
                Sleep(1);
            } while (IsProcRunning(procInfo.hProcess));
            ReadPipe(stdOutRead, rawData, logData, LogType::Info, settings);
            ReadPipe(stdErrRead, rawData, logData, LogType::Error, settings);
        }
        else
        {
            WaitForSingleObject(procInfo.hProcess, INFINITE);
        }

        DWORD exitCode;
        if (GetExitCodeProcess(procInfo.hProcess, &exitCode) != 0)
            result = exitCode;

        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);

        // Cleanup
    ERROR_EXIT:
        if (startupInfoEx.StartupInfo.hStdOutput != nullptr)
            CloseHandle(startupInfoEx.StartupInfo.hStdOutput);
        if (startupInfoEx.StartupInfo.hStdError != nullptr)
            CloseHandle(startupInfoEx.StartupInfo.hStdError);
        if (stdOutRead != nullptr)
            CloseHandle(stdOutRead);
        if (stdErrRead != nullptr)
            CloseHandle(stdErrRead);
        if (startupInfoEx.lpAttributeList != nullptr)
        {
            DeleteProcThreadAttributeList(startupInfoEx.lpAttributeList);
            Allocator::Free(startupInfoEx.lpAttributeList);
        }
        if (environmentStr)
            Allocator::Free(environmentStr);
    }

    return result;
}

Window* WindowsPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    return New<WindowsWindow>(settings);
}

void* WindowsPlatform::LoadLibrary(const Char* filename)
{
    ASSERT(filename);
    PROFILE_CPU();
    ZoneText(filename, StringUtils::Length(filename));

    // Add folder to search path to load dependency libraries
    StringView folder = StringUtils::GetDirectoryName(filename);
    if (folder.HasChars() && FileSystem::IsRelative(folder))
        folder = StringView::Empty;
    if (folder.HasChars())
    {
        const String folderNullTerminated(folder);
        AddDllDirectory(folderNullTerminated.Get());
    }

    // Avoiding windows dialog boxes if missing
    const DWORD errorMode = SEM_NOOPENFILEERRORBOX;
    DWORD prevErrorMode = 0;
    const BOOL hasErrorMode = SetThreadErrorMode(errorMode, &prevErrorMode);

    // Ensure that dll is properly searched
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);

    // Load the library
    void* handle = ::LoadLibraryW(filename);
    if (!handle)
    {
        LOG(Warning, "Failed to load '{0}' (GetLastError={1})", filename, GetLastError());
    }

    if (hasErrorMode)
    {
        SetThreadErrorMode(prevErrorMode, nullptr);
    }

#if CRASH_LOG_ENABLE
    // Refresh modules info during next stack trace collecting to have valid debug symbols information
    FlaxDbgHelpLock();
    if (folder.HasChars() && !SymbolsPath.Contains(folder))
    {
        SymbolsPath.Add(folder);
        SymbolsPath.Last().Replace('/', '\\');
        OnSymbolsPathModified();
    }
    FlaxDbgHelpUnlock();
#endif

    return handle;
}

#if CRASH_LOG_ENABLE

Array<PlatformBase::StackFrame> WindowsPlatform::GetStackFrames(int32 skipCount, int32 maxDepth, void* context)
{
    Array<StackFrame> result;
    FlaxDbgHelpLock();

    // Initialize
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    if (!SymInitialized)
    {
        SymInitialized = true;
        String symbolSearchPath;
        for (auto& path : SymbolsPath)
        {
            symbolSearchPath += path;
            symbolSearchPath += ";";
        }
        symbolSearchPath += Platform::GetWorkingDirectory();
        SymInitializeW(process, *symbolSearchPath, TRUE);
    }

    // Capture the context if missing
    /*EXCEPTION_POINTERS exceptionPointers;
    CONTEXT contextData;
    if (!context)
    {
        exceptionPointers.ExceptionRecord = nullptr;
        exceptionPointers.ContextRecord = &contextData;
        MemoryClear(&contextData, sizeof(CONTEXT));
        contextData.ContextFlags = CONTEXT_FULL;
        RtlCaptureContext(&contextData);
        context = &exceptionPointers;
    }*/

    // Capture the backtrace
    int32 count;
    void* backtrace[100];
    maxDepth = Math::Min<int32>(maxDepth, ARRAY_COUNT(backtrace));
    if (context)
    {
        auto& ctx = *(((EXCEPTION_POINTERS*)context)->ContextRecord);
        STACKFRAME64 stack;
        memset(&stack, 0, sizeof(stack));
        DWORD imageType;
#ifdef _M_IX86
        imageType = IMAGE_FILE_MACHINE_I386;
        stack.AddrPC.Offset = ctx.Eip;
        stack.AddrPC.Mode = AddrModeFlat;
        stack.AddrFrame.Offset = ctx.Ebp;
        stack.AddrFrame.Mode = AddrModeFlat;
        stack.AddrStack.Offset = ctx.Esp;
        stack.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
        imageType = IMAGE_FILE_MACHINE_AMD64;
        stack.AddrPC.Offset = ctx.Rip;
        stack.AddrPC.Mode = AddrModeFlat;
        stack.AddrFrame.Offset = ctx.Rsp;
        stack.AddrFrame.Mode = AddrModeFlat;
        stack.AddrStack.Offset = ctx.Rsp;
        stack.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
        imageType = IMAGE_FILE_MACHINE_IA64;
        stack.AddrPC.Offset = ctx.StIIP;
        stack.AddrPC.Mode = AddrModeFlat;
        stack.AddrFrame.Offset = ctx.IntSp;
        stack.AddrFrame.Mode = AddrModeFlat;
        stack.AddrBStore.Offset = ctx.RsBSP;
        stack.AddrBStore.Mode = AddrModeFlat;
        stack.AddrStack.Offset = ctx.IntSp;
        stack.AddrStack.Mode = AddrModeFlat;
#elif _M_ARM64
        imageType = IMAGE_FILE_MACHINE_ARM64;
        stack.AddrPC.Offset = ctx.Pc;
        stack.AddrPC.Mode = AddrModeFlat;
        stack.AddrFrame.Offset = ctx.Fp;
        stack.AddrFrame.Mode = AddrModeFlat;
        stack.AddrStack.Offset = ctx.Sp;
        stack.AddrStack.Mode = AddrModeFlat;
#else
#error "Platform not supported!"
#endif
        count = 0;
        for (int32 i = 0; i < skipCount; i++)
            StackWalk64(imageType, process, thread, &stack, &ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr);
        while (count < maxDepth && StackWalk64(imageType, process, thread, &stack, &ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            backtrace[count++] = (void*)stack.AddrPC.Offset;
    }
    else
    {
        skipCount++;
        count = RtlCaptureStackBackTrace(skipCount, maxDepth, backtrace, nullptr);
    }

    // Walk the stack to collect the symbols
    result.Resize(count);
    Platform::MemoryClear(result.Get(), count * sizeof(StackFrame));
    for (int32 i = 0; i < count; i++)
    {
        auto& frame = result[i];
        frame.ProgramCounter = backtrace[i];

        // Get function name
        alignas(IMAGEHLP_SYMBOL64) byte symbolData[sizeof(IMAGEHLP_SYMBOL64) + ARRAY_COUNT(frame.FunctionName)];
        IMAGEHLP_SYMBOL64* symbol = (IMAGEHLP_SYMBOL64*)symbolData;
        symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        symbol->MaxNameLength = ARRAY_COUNT(frame.FunctionName) - 1;
        DWORD64 displacement = 0;
        if (SymGetSymFromAddr64(process, (DWORD64)frame.ProgramCounter, &displacement, symbol))
        {
            UnDecorateSymbolName(symbol->Name, frame.FunctionName, ARRAY_COUNT(frame.FunctionName), UNDNAME_COMPLETE);
        }

        // Get filename and line number
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD offset;
        if (SymGetLineFromAddr64(process, (DWORD64)frame.ProgramCounter, &offset, &line))
        {
            frame.LineNumber = line.LineNumber;
            const int32 fileNameLength = Math::Min<int32>(ARRAY_COUNT(frame.FileName) - 1, StringUtils::Length(line.FileName));
            memcpy(frame.FileName, line.FileName, fileNameLength);
            frame.FileName[fileNameLength] = '\0';
        }

        // Get module name
        IMAGEHLP_MODULE64 module;
        module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        if (SymGetModuleInfo64(process, (DWORD64)frame.ProgramCounter, &module))
        {
            const int32 moduleNameLength = Math::Min<int32>(ARRAY_COUNT(frame.ModuleName) - 1, StringUtils::Length(module.ImageName));
            memcpy(frame.ModuleName, module.ImageName, moduleNameLength);
            frame.ModuleName[moduleNameLength] = '\0';
        }
    }

    FlaxDbgHelpUnlock();
    return result;
}

void WindowsPlatform::CollectCrashData(const String& crashDataFolder, void* context)
{
    // Create mini dump file for crash debugging
    struct CrashInfo
    {
        LPEXCEPTION_POINTERS ExceptionPointers;
        DWORD CallerThreadId;
        String MiniDumpPath;
    };
    CrashInfo crashInfo;
    crashInfo.ExceptionPointers = (PEXCEPTION_POINTERS)context;
    crashInfo.CallerThreadId = GetCurrentThreadId();
    crashInfo.MiniDumpPath = crashDataFolder / TEXT("Minidump.dmp");
    LOG(Error, "Creating Mini Dump to {0}", crashInfo.MiniDumpPath);
    auto threadFunc = [](void* data) -> DWORD
    {
        auto info = (CrashInfo*)data;
        const HANDLE process = GetCurrentProcess();
        const HANDLE file = CreateFileW(*info->MiniDumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        const MINIDUMP_TYPE minidumpType = (MINIDUMP_TYPE)(MiniDumpWithFullMemoryInfo | MiniDumpFilterMemory | MiniDumpWithHandleData | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
        MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInfo;
        minidumpExceptionInfo.ThreadId = info->CallerThreadId;
        minidumpExceptionInfo.ExceptionPointers = info->ExceptionPointers;
        minidumpExceptionInfo.ClientPointers = FALSE;
        MiniDumpWriteDump(process, GetProcessId(process), file, minidumpType, info->ExceptionPointers ? &minidumpExceptionInfo : nullptr, nullptr, nullptr);
        CloseHandle(file);
        return 0;
    };
    DWORD threadID;
    const auto handle = CreateThread(0, 0x8000, threadFunc, &crashInfo, 0, &threadID);
    WaitForSingleObject(handle, INFINITE);
}

#endif

#endif
