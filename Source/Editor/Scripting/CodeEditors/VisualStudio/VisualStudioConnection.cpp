// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_VISUAL_STUDIO_DTE

// Import EnvDTE
#pragma warning(disable : 4278)
#pragma warning(disable : 4146)
#import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("8.0") lcid("0") raw_interfaces_only named_guids
#pragma warning(default : 4146)
#pragma warning(default : 4278)

// Import Microsoft.VisualStudio.Setup.Configuration.Native
#include <Microsoft.VisualStudio.Setup.Configuration.Native/Setup.Configuration.h>
#pragma comment(lib, "Microsoft.VisualStudio.Setup.Configuration.Native.lib")

#include "VisualStudioConnection.h"
#include "Engine/Platform/Windows/ComPtr.h"

/// <summary>
/// Handles retrying of calls that fail to access Visual Studio.
/// This is due to the weird nature of VS when calling its methods from external code.
/// If this message filter isn't registered some calls will just fail silently.
/// </summary>
class VSMessageFilter : public IMessageFilter
{
private:

	LONG _refCount;

public:

	VSMessageFilter()
	{
		_refCount = 0;
	}

	virtual ~VSMessageFilter()
	{
	}

public:

	DWORD __stdcall HandleInComingCall(DWORD dwCallType, HTASK htaskCaller, DWORD dwTickCount, LPINTERFACEINFO lpInterfaceInfo) override
	{
		return SERVERCALL_ISHANDLED;
	}

	DWORD __stdcall RetryRejectedCall(HTASK htaskCallee, DWORD dwTickCount, DWORD dwRejectType) override
	{
		if (dwRejectType == SERVERCALL_RETRYLATER)
		{
			// Retry immediately
			return 99;
		}

		// Cancel the call
		return -1;
	}

	DWORD __stdcall MessagePending(HTASK htaskCallee, DWORD dwTickCount, DWORD dwPendingType) override
	{
		return PENDINGMSG_WAITDEFPROCESS;
	}

	//	COM requirement. Returns instance of an interface of provided type.
	HRESULT __stdcall QueryInterface(REFIID iid, void** ppvObject) override
	{
		if (iid == IID_IDropTarget || iid == IID_IUnknown)
		{
			AddRef();
			*ppvObject = this;
			return S_OK;
		}

		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	// COM requirement. Increments objects reference count.
	ULONG __stdcall AddRef() override
	{
		return _InterlockedIncrement(&_refCount);
	}

	//	COM requirement. Decreases the objects reference count and deletes the object if its zero.
	ULONG __stdcall Release() override
	{
		LONG count = _InterlockedDecrement(&_refCount);

		if (count == 0)
		{
			delete this;
			return 0;
		}

		return count;
	}
};

// Helper macro
#define CHECK_VS_RESULT(target) if (FAILED(result)) return #target " failed with result: " + std::to_string((int)result);

#define USE_ITEM_OPERATIONS_OPEN 0
#define USE_PROJECT_ITEM_OPEN 1
#define USE_DOCUMENT_OPEN 0

class LocalBSTR
{
public:

	BSTR Str;

public:

	LocalBSTR()
	{
		Str = nullptr;
	}

	LocalBSTR(const LPSTR str)
	{
		const auto wslen = MultiByteToWideChar(CP_ACP, 0, str, (int)strlen(str), nullptr, 0);
		Str = SysAllocStringLen(0, wslen);
		MultiByteToWideChar(CP_ACP, 0, str, (int)strlen(str), Str, wslen);
	}

	LocalBSTR(const wchar_t* str)
	{
		Str = ::SysAllocString(str);
	}

	~LocalBSTR()
	{
		if (Str)
		{
			::SysFreeString(Str);
		}
	}

public:

	operator const BSTR() const
	{
		return Str;
	}

	operator const wchar_t*() const
	{
		return Str;
	}
};

namespace VisualStudio
{
	bool AreFilePathsEqual(const wchar_t* path1, const wchar_t* path2)
    {
		return _wcsicmp(path1, path2) == 0;
    }

	class ConnectionInternal
	{
	public:

		const wchar_t* ClsID;
		LocalBSTR SolutionPath;

		CLSID CLSID;
		ComPtr<EnvDTE::_DTE> DTE;

	public:

		ConnectionInternal(const wchar_t* clsID, const wchar_t* solutionPath)
			: ClsID(clsID)
			, SolutionPath(solutionPath)
		{
		}

	public:

		bool IsValid() const
		{
			return DTE != nullptr;
		}
	};

	static Result FindRunningInstance(ConnectionHandle connection)
	{
		HRESULT result;

		ComPtr<IRunningObjectTable> runningObjectTable;
		result = GetRunningObjectTable(0, &runningObjectTable);
		CHECK_VS_RESULT("VisualStudio::FindRunningInstance - GetRunningObjectTable");

		ComPtr<IEnumMoniker> enumMoniker;
		result = runningObjectTable->EnumRunning(&enumMoniker);
		CHECK_VS_RESULT("VisualStudio::FindRunningInstance - EnumRunning");

		ComPtr<IMoniker> dteMoniker;
		result = CreateClassMoniker(connection->CLSID, &dteMoniker);
		CHECK_VS_RESULT("VisualStudio::FindRunningInstance - CreateClassMoniker");

		ComPtr<IMoniker> moniker;
		ULONG count = 0;
		while (enumMoniker->Next(1, &moniker, &count) == S_OK)
		{
			if (moniker->IsEqual(dteMoniker))
			{
				ComPtr<IUnknown> curObject;
				result = runningObjectTable->GetObjectW(moniker, &curObject);
				moniker.Detach();

				if (result != S_OK)
					continue;

				ComPtr<EnvDTE::_DTE> dte;
				curObject->QueryInterface(__uuidof(EnvDTE::_DTE), (void**)&dte);

				if (dte == nullptr)
					continue;

				ComPtr<EnvDTE::_Solution> solution;
				if (FAILED(dte->get_Solution(&solution)))
					continue;

				LocalBSTR fullName;
				if (FAILED(solution->get_FullName(&fullName.Str)))
					continue;

				if (AreFilePathsEqual(connection->SolutionPath, fullName))
				{
					// Found
					connection->DTE = dte;
					break;
				}
			}
		}

		return Result::Ok;
	}

	// Opens a new Visual Studio instance of the specified version with the provided solution.
	//
	// @param[in]	clsID			Class ID of the specific Visual Studio version to start.
	// @param[in]	solutionPath	Path to the solution the instance needs to open.
	static Result OpenInstance(ConnectionHandle connection)
	{
		HRESULT result;

		ComPtr<IUnknown> newInstance = nullptr;
		result = CoCreateInstance(connection->CLSID, nullptr, CLSCTX_LOCAL_SERVER, EnvDTE::IID__DTE, (LPVOID*)&newInstance);
		CHECK_VS_RESULT("VisualStudio::OpenInstance - CoCreateInstance");

		newInstance->QueryInterface(&connection->DTE);
		if (connection->DTE == nullptr)
			return "Invalid DTE handle";

		connection->DTE->put_UserControl(TRUE);

		ComPtr<EnvDTE::_Solution> solution;
		result = connection->DTE->get_Solution(&solution);
		CHECK_VS_RESULT("VisualStudio::OpenInstance - dte->get_Solution");

		result = solution->Open(connection->SolutionPath);
		CHECK_VS_RESULT("VisualStudio::OpenInstance - solution->Open");

		// Wait until VS opens
		int maxWaitMs = 10 * 1000;
		int elapsedMs = 0;
		int stepTimeMs = 100;
		while (elapsedMs < maxWaitMs)
		{
			ComPtr<EnvDTE::Window> window = nullptr;
			if (SUCCEEDED(connection->DTE->get_MainWindow(&window)))
				return Result::Ok;

			Sleep(stepTimeMs);
			elapsedMs += stepTimeMs;
		}

		return "Visual Studio open timout";
	}

    static ComPtr<EnvDTE::ProjectItem> FindItem(const ComPtr<EnvDTE::_Solution>& solution, BSTR filePath)
    {
        HRESULT result;

        ComPtr<EnvDTE::ProjectItem> projectItem;
        result = solution->FindProjectItem(filePath, &projectItem);
        if (FAILED(result))
            return nullptr;

        return projectItem;
    }

	// Opens a file on a specific line in a running Visual Studio instance.
	//
	// @param[in]	dte			DTE object retrieved from FindRunningInstance() or OpenInstance().
	// @param[in]	filePath	Path of the file to open. File should be a part of the VS solution.
	// @param[in]	line		Line on which to focus Visual Studio after the file is open.
	static Result OpenFile(ConnectionHandle handle, BSTR filePath, unsigned int line)
	{
		HRESULT result;
		LocalBSTR viewKind(EnvDTE::vsViewKindPrimary);

#if USE_ITEM_OPERATIONS_OPEN
		ComPtr<EnvDTE::ItemOperations> itemOperations;
		result = handle->DTE->get_ItemOperations(&itemOperations);
		CHECK_VS_RESULT("VisualStudio::OpenFile - DTE->get_ItemOperations");
#endif

		// Check if that file is opened
		VARIANT_BOOL isOpen = 0;
#if USE_ITEM_OPERATIONS_OPEN
		result = itemOperations->IsFileOpen(filePath, viewKind.Str, &isOpen);
		CHECK_VS_RESULT("VisualStudio::OpenFile - itemOperations->IsFileOpen");
#else
		result = handle->DTE->get_IsOpenFile(viewKind.Str, filePath, &isOpen);
		CHECK_VS_RESULT("VisualStudio::OpenFile - DTE->get_IsOpenFile");
#endif

		// Open or navigate to a window with a file
		ComPtr<EnvDTE::Window> window;
		ComPtr<EnvDTE::Document> document;
		if (isOpen == 0)
		{
			// Open file
#if USE_DOCUMENT_OPEN
			ComPtr<EnvDTE::Documents> documents;
			result = handle->DTE->get_Documents(&documents);
			CHECK_VS_RESULT("VisualStudio::OpenInstance - DTE->get_Documents");

			ComPtr<EnvDTE::Document> tmp;
			LocalBSTR kind(_T("Auto"));
			result = documents->Open(filePath, kind.Str, VARIANT_FALSE, &tmp);
			CHECK_VS_RESULT("VisualStudio::OpenInstance - documents->Open");

			result = tmp->get_ActiveWindow(&window);
			CHECK_VS_RESULT("VisualStudio::OpenFile - tmp->get_ActiveWindow");
#elif USE_PROJECT_ITEM_OPEN
			ComPtr<EnvDTE::_Solution> solution;
			result = handle->DTE->get_Solution(&solution);
			CHECK_VS_RESULT("VisualStudio::OpenInstance - DTE->get_Solution");
            
            ComPtr<EnvDTE::ProjectItem> projectItem = FindItem(solution, filePath);
            if (projectItem)
			{
				result = projectItem->Open(viewKind, &window);
				CHECK_VS_RESULT("VisualStudio::OpenFile - projectItem->Open");
			}
#elif USE_ITEM_OPERATIONS_OPEN
			result = itemOperations->OpenFile(filePath, viewKind, &window);
			CHECK_VS_RESULT("VisualStudio::OpenFile - itemOperations->OpenFile");
#else
			result = handle->DTE->OpenFile(viewKind, filePath, &window);
			CHECK_VS_RESULT("VisualStudio::OpenFile - DTE->OpenFile");
#endif
			if (window == nullptr)
				return Result::Ok;

			// Activate window and get document handle
			result = window->Activate();
			CHECK_VS_RESULT("VisualStudio::OpenFile - window->Activate");
			result = handle->DTE->get_ActiveDocument(&document);
			CHECK_VS_RESULT("VisualStudio::OpenFile - dte->get_ActiveDocument");
		}
		else
		{
			// Find opened document

			ComPtr<EnvDTE::Documents> documents;
			result = handle->DTE->get_Documents(&documents);
			CHECK_VS_RESULT("VisualStudio::OpenFile - DTE->get_Documents");

			long documentsCount;
			result = documents->get_Count(&documentsCount);
			CHECK_VS_RESULT("VisualStudio::OpenFile - documents->get_Count");

			for (int i = 1; i <= documentsCount; i++) // They are counting from [1..Count]
			{
				ComPtr<EnvDTE::Document> tmp;
				result = documents->Item(_variant_t(i), &tmp);
				CHECK_VS_RESULT("VisualStudio::OpenFile - documents->Item");
				if (tmp == nullptr)
					continue;

				BSTR tmpPath;
				result = tmp->get_FullName(&tmpPath);
				CHECK_VS_RESULT("VisualStudio::OpenFile - tmp->get_FullName");

				if (AreFilePathsEqual(filePath, tmpPath))
				{
					result = tmp->Activate();
					CHECK_VS_RESULT("VisualStudio::OpenFile - tmp->Activate");

					// Found
					document = tmp;
					break;
				}
			}
		}
		if (document == nullptr)
			return "Cannot open a file";

		// Check if need to select a given line
		if (line != 0)
		{
			ComPtr<IDispatch> selection;
			result = document->get_Selection(&selection);
			CHECK_VS_RESULT("VisualStudio::OpenFile - activeDocument->get_Selection");
			if (selection == nullptr)
				return Result::Ok;

			ComPtr<EnvDTE::TextSelection> textSelection;
			result = selection->QueryInterface(&textSelection);
			CHECK_VS_RESULT("VisualStudio::OpenFile - selection->QueryInterface");

			textSelection->GotoLine(line, VARIANT_TRUE);
		}

		/*
		// Bring the window in focus
		window = nullptr;
		if (SUCCEEDED(dte->get_MainWindow(&window)))
		{
			window->Activate();

			HWND hWnd;
			window->get_HWnd((LONG*)&hWnd);
			SetForegroundWindow(hWnd);
		}
		*/

		return Result::Ok;
	}

	// Adds a file to the project opened in a running Visual Studio instance.
	//
	// @param[in]	dte			DTE object retrieved from FindRunningInstance() or OpenInstance().
	// @param[in]	filePath	Path of the file to add.
	// @param[in]	localPath	Path of the file to add relative to the solution folder.
	static Result AddFile(ConnectionHandle handle, BSTR filePath, const wchar_t* localPath)
	{
		HRESULT result;

		ComPtr<EnvDTE::_Solution> solution;
		result = handle->DTE->get_Solution(&solution);
		CHECK_VS_RESULT("VisualStudio::OpenInstance - DTE->get_Solution");
        
        ComPtr<EnvDTE::ProjectItem> projectItem = FindItem(solution, filePath);
        if (projectItem)
        {
            // Already added
            return Result::Ok;
        }
   
        ComPtr<EnvDTE::Projects> projects;
        result = solution->get_Projects(&projects);
        if (FAILED(result))
            return nullptr;

        long projectsCount = 0;
        result = projects->get_Count(&projectsCount);
        if (FAILED(result))
            return nullptr;

	    ComPtr<EnvDTE::Project> project;
	    wchar_t buffer[500];

        // Place .Build.cs scripts into BuildScripts project
        const int localPathLength = (int)wcslen(localPath);
        if (localPathLength >= 10 && _wcsicmp(localPath + localPathLength - ARRAY_COUNT(".Build.cs") + 1, TEXT(".Build.cs")) == 0)
        {
            for (long projectIndex = 1; projectIndex <= projectsCount; projectIndex++) // They are counting from [1..Count]
            {
	            result = projects->Item(_variant_t(projectIndex), &project);
	            if (FAILED(result) || !project)
		            continue;
                
				_bstr_t name;
                if (SUCCEEDED(project->get_Name(name.GetAddress())) && wcscmp(name, TEXT("BuildScripts")) == 0)
                    break;
                project = nullptr;
            }
        }
        else
        {
            // TODO: find the project to add script to? for .cs scripts it should be csproj, for c++ vcproj?

            // Find parent folder
            wchar_t* path = filePath + wcslen(localPath) + 1;
            int length = (int)wcslen(path);
            memcpy(buffer, path, length * sizeof(wchar_t));
            for (int i = length - 1; i >= 0; i--)
            {
                if (buffer[i] == '\\' || buffer[i] == '/')
                {
                    buffer[i] = '\0';
                    length = i;
                    break;
                }
            }
            const LocalBSTR parentPath(TEXT("MyProject"));
            projectItem = FindItem(solution, parentPath);
            // ...
        }

        // TODO: add file and all missing parent folders to the project

		return Result::Ok;
	}

	class CleanupHelper
	{
	public:

		IMessageFilter* oldFilter;
		VSMessageFilter* newFilter;

		CleanupHelper()
		{
			CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			newFilter = new VSMessageFilter();
			CoRegisterMessageFilter(newFilter, &oldFilter);
		}

		~CleanupHelper()
		{
			CoRegisterMessageFilter(oldFilter, nullptr);
			CoUninitialize();
		}
	};
};

VisualStudio::Result VisualStudio::Result::Ok;

int VisualStudio::GetVisualStudioVersions(InstanceInfo* infos, int infosCount)
{
    // Try to create the CoCreate the class; if that fails, likely no instances are registered
	ComPtr<ISetupConfiguration2> query;
	HRESULT result = CoCreateInstance(__uuidof(SetupConfiguration), nullptr, CLSCTX_ALL, __uuidof(ISetupConfiguration2), (LPVOID*)&query);
	if (FAILED(result))
	{
		return 0;
	}

	// Get the enumerator
	ComPtr<IEnumSetupInstances> enumSetupInstances;
	result = query->EnumAllInstances(&enumSetupInstances);
	if (FAILED(result))
	{
		return 0;
	}

	// Check the state and version of the enumerated instances
    int32 count = 0;
    ComPtr<ISetupInstance> instance;
	while(count < infosCount)
    {
        ULONG fetched = 0;
        result = enumSetupInstances->Next(1, &instance, &fetched);
        if (FAILED(result) || fetched == 0)
        {
            break;
        }

        ComPtr<ISetupInstance2> setupInstance2;
        result = instance->QueryInterface(__uuidof(ISetupInstance2), (LPVOID*)&setupInstance2);
        if (SUCCEEDED(result))
        {
            InstanceState state;
            result = setupInstance2->GetState(&state);
            if (SUCCEEDED(result) && (state & eLocal) != 0)
            {
                BSTR installationVersion;
                result = setupInstance2->GetInstallationVersion(&installationVersion);
                if (SUCCEEDED(result))
                {
                    BSTR installationPath;
                    result = setupInstance2->GetInstallationPath(&installationPath);
                    if (SUCCEEDED(result))
                    {
                        BSTR productPath;
                        result = setupInstance2->GetProductPath(&productPath);
                        if (SUCCEEDED(result))
                        {
                            auto& info = infos[count++];

                            char version[3];
                            version[0] = (char)installationVersion[0];
                            version[1] = (char)installationVersion[1];
                            version[2] = 0;
                            info.VersionMajor = atoi(version);

                            swprintf_s(info.ExecutablePath, MAX_PATH, L"%s\\%s", installationPath, productPath);

                            wchar_t progID[100];
                            swprintf_s(progID, 100, L"VisualStudio.DTE.%d.0", info.VersionMajor);

                            CLSID clsid;
                            CLSIDFromProgID(progID, &clsid);
                            StringFromGUID2(clsid, info.CLSID, 40);

                            SysFreeString(productPath);
                        }
                        SysFreeString(installationPath);
                    }
                    SysFreeString(installationVersion);
                }
            }
        }
    }

    return count;
}

void VisualStudio::OpenConnection(ConnectionHandle& connection, const wchar_t* clsID, const wchar_t* solutionPath)
{
	connection = new ConnectionInternal(clsID, solutionPath);
}

void VisualStudio::CloseConnection(ConnectionHandle& connection)
{
	if (connection)
	{
		delete connection;
		connection = nullptr;
	}
}

bool VisualStudio::IsActive(const ConnectionHandle& connection)
{
	// Check if already opened
	if (connection->IsValid())
		return true;

	// Try to find active
	auto e = FindRunningInstance(connection);
	return connection->DTE != nullptr;
}

VisualStudio::Result VisualStudio::OpenSolution(ConnectionHandle connection)
{
	// Check if already opened
	if (connection->IsValid())
		return Result::Ok;

	// Temporary data
	CleanupHelper helper;
	HRESULT result;

	// Cache VS version CLSID
	result = CLSIDFromString(connection->ClsID, &connection->CLSID);
	CHECK_VS_RESULT("VisualStudio::CLSIDFromString");

	// Get or open VS with solution
	auto e = FindRunningInstance(connection);
	if (e.Failed())
		return e;
	if (connection->DTE == nullptr)
	{
		e = OpenInstance(connection);
		if (e.Failed())
			return e;
		if (connection->DTE == nullptr)
			return "Cannot open Visual Studio";
	}

	// Focus VS main window
	ComPtr<EnvDTE::Window> window;
	if (SUCCEEDED(connection->DTE->get_MainWindow(&window)))
		window->Activate();

	return Result::Ok;
}

VisualStudio::Result VisualStudio::OpenFile(ConnectionHandle connection, const wchar_t* path, unsigned int line)
{
	// Ensure to have valid connection
	auto result = OpenSolution(connection);
	if (result.Failed())
		return result;

	// Open file
	CleanupHelper helper;
    const LocalBSTR pathBstr(path);
	return OpenFile(connection, pathBstr.Str, line);
}

VisualStudio::Result VisualStudio::AddFile(ConnectionHandle connection, const wchar_t* path, const wchar_t* localPath)
{
	// Ensure to have valid connection
	auto result = OpenSolution(connection);
	if (result.Failed())
		return result;

	// Add file
	CleanupHelper helper;
	LocalBSTR pathBstr(path);
	return AddFile(connection, pathBstr.Str, localPath);
}

#endif
