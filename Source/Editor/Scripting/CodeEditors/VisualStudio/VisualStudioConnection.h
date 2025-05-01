// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if USE_VISUAL_STUDIO_DTE

#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include <string>

/// <summary>
/// Contains various helper classes for interacting with a Visual Studio instance running on this machine.
/// </summary>
namespace VisualStudio
{
	/// <summary>
	/// Visual Studio connection operation result
	/// </summary>
	struct Result
	{
		static Result Ok;

		std::string Message;

		Result()
		{
			Message = "";
		}

		Result(const char* msg)
		{
			Message = msg;
		}

		Result(const std::string& msg)
		{
			Message = msg;
		}

		bool Failed() const
		{
			return Message.size() > 0;
		}
	};

    struct InstanceInfo
    {
        wchar_t CLSID[40];
        wchar_t ExecutablePath[MAX_PATH];
        int VersionMajor;
    };

	class ConnectionInternal;
	typedef ConnectionInternal* ConnectionHandle;

    int GetVisualStudioVersions(InstanceInfo* infos, int infosCount);
	void OpenConnection(ConnectionHandle& connection, const wchar_t* clsID, const wchar_t* solutionPath);
	void CloseConnection(ConnectionHandle& connection);
	bool IsActive(const ConnectionHandle& connection);
	Result OpenSolution(ConnectionHandle connection);
	Result OpenFile(ConnectionHandle connection, const wchar_t* path, unsigned int line);
	Result AddFile(ConnectionHandle connection, const wchar_t* path, const wchar_t* localPath);

	/// <summary>
	/// Visual Studio connection wrapper class
	/// </summary>
	class Connection
	{
	private:

		ConnectionHandle Handle;

	public:

		Connection(const wchar_t* clsID, const wchar_t* solutionPath)
		{
			OpenConnection(Handle, clsID, solutionPath);
		}

		~Connection()
		{
			CloseConnection(Handle);
		}

	public:

		bool IsActive() const
		{
			return VisualStudio::IsActive(Handle);
		}

		Result OpenSolution() const
		{
			return VisualStudio::OpenSolution(Handle);
		}

		Result OpenFile(const wchar_t* path, unsigned int line) const
		{
			return VisualStudio::OpenFile(Handle, path, line);
		}

        Result AddFile(const wchar_t* path, const wchar_t* localPath) const
		{
			return VisualStudio::AddFile(Handle, path, localPath);
		}
	};
}

#endif
