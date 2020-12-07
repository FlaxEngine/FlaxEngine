/**
 * \file
 * Copyright 2016 Microsoft
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
#ifndef __MONO_METADATA_PROCESS_INTERNALS_H__
#define __MONO_METADATA_PROCESS_INTERNALS_H__

#include <config.h>
#include <glib.h>

#if G_HAVE_API_SUPPORT(HAVE_UWP_WINAPI_SUPPORT)
#include <windows.h>
#endif

// On platforms not using classic WIN API support the  implementation of bellow methods are hosted in separate source file
// process-windows-*.c. On platforms using classic WIN API the implementation is still keept in process.c and still declared
// static and in some places even inlined.
#if !G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) && !G_HAVE_API_SUPPORT(HAVE_UWP_WINAPI_SUPPORT) && !G_HAVE_API_SUPPORT(HAVE_GAMES_WINAPI_SUPPORT)

#include <winnt.h>

typedef struct _PROCESS_INFORMATION {
	HANDLE hProcess;
	HANDLE hThread;
	DWORD dwProcessId;
	DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

typedef struct _STARTUPINFOA {
	DWORD   cb;
	LPSTR   lpReserved;
	LPSTR   lpDesktop;
	LPSTR   lpTitle;
	DWORD   dwX;
	DWORD   dwY;
	DWORD   dwXSize;
	DWORD   dwYSize;
	DWORD   dwXCountChars;
	DWORD   dwYCountChars;
	DWORD   dwFillAttribute;
	DWORD   dwFlags;
	WORD    wShowWindow;
	WORD    cbReserved2;
	LPBYTE  lpReserved2;
	HANDLE  hStdInput;
	HANDLE  hStdOutput;
	HANDLE  hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;
typedef struct _STARTUPINFOW {
	DWORD   cb;
	LPWSTR  lpReserved;
	LPWSTR  lpDesktop;
	LPWSTR  lpTitle;
	DWORD   dwX;
	DWORD   dwY;
	DWORD   dwXSize;
	DWORD   dwYSize;
	DWORD   dwXCountChars;
	DWORD   dwYCountChars;
	DWORD   dwFillAttribute;
	DWORD   dwFlags;
	WORD    wShowWindow;
	WORD    cbReserved2;
	LPBYTE  lpReserved2;
	HANDLE  hStdInput;
	HANDLE  hStdOutput;
	HANDLE  hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;
#ifdef UNICODE
typedef STARTUPINFOW STARTUPINFO;
typedef LPSTARTUPINFOW LPSTARTUPINFO;
#else
typedef STARTUPINFOA STARTUPINFO;
typedef LPSTARTUPINFOA LPSTARTUPINFO;
#endif // UNICODE

#endif /* !G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) && !G_HAVE_API_SUPPORT(HAVE_UWP_WINAPI_SUPPORT) && !G_HAVE_API_SUPPORT(HAVE_GAMES_WINAPI_SUPPORT)) */

#if !G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)
void
mono_w32process_get_fileversion (MonoObject *filever, gunichar2 *filename, MonoError *error);

void
mono_process_init_startup_info (HANDLE stdin_handle, HANDLE stdout_handle,
				HANDLE stderr_handle,STARTUPINFO *startinfo);

gboolean
mono_process_create_process (MonoCreateProcessCoop *coop, MonoW32ProcessInfo *mono_process_info,
	MonoStringHandle cmd, guint32 creation_flags, gunichar2 *env_vars, gunichar2 *dir, STARTUPINFO *start_info,
	PROCESS_INFORMATION *process_info);

MonoBoolean
mono_icall_get_process_working_set_size (gpointer handle, gsize *min, gsize *max);

MonoBoolean
mono_icall_set_process_working_set_size (gpointer handle, gsize min, gsize max);

gint32
mono_icall_get_priority_class (gpointer handle);

MonoBoolean
mono_icall_set_priority_class (gpointer handle, gint32 priorityClass);

gboolean
mono_process_win_enum_processes (DWORD *pids, DWORD count, DWORD *needed);
#endif  /* !G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) */

#endif /* __MONO_METADATA_PROCESS_INTERNALS_H__ */
