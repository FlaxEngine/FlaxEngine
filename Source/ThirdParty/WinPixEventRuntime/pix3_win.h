/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       PIX3_win.h
 *  Content:    PIX include file
 *              Don't include this file directly - use pix3.h
 *
 ****************************************************************************/

#pragma once

#ifndef _PIX3_H_
#error Don't include this file directly - use pix3.h
#endif

#ifndef _PIX3_WIN_H_
#define _PIX3_WIN_H_

 // PIXEventsThreadInfo is defined in PIXEventsCommon.h
struct PIXEventsThreadInfo;

extern "C" PIXEventsThreadInfo* PIXGetThreadInfo();

#if defined(USE_PIX) && defined(USE_PIX_SUPPORTED_ARCHITECTURE)
// Notifies PIX that an event handle was set as a result of a D3D12 fence being signaled.
// The event specified must have the same handle value as the handle
// used in ID3D12Fence::SetEventOnCompletion.
extern "C" void WINAPI PIXNotifyWakeFromFenceSignal(_In_ HANDLE event);
#endif

// The following defines denote the different metadata values that have been used
// by tools to denote how to parse pix marker event data. The first two values
// are legacy values.
#define WINPIX_EVENT_UNICODE_VERSION 0
#define WINPIX_EVENT_ANSI_VERSION 1
#define WINPIX_EVENT_PIX3BLOB_VERSION 2

#define D3D12_EVENT_METADATA WINPIX_EVENT_PIX3BLOB_VERSION

__forceinline UINT64 PIXGetTimestampCounter()
{
    LARGE_INTEGER time = {};
    QueryPerformanceCounter(&time);
    return time.QuadPart;
}

template<class T>
void PIXCopyEventArgument(UINT64*&, const UINT64*, T);

template<class T>
void PIXStoreContextArgument(UINT64*& destination, const UINT64* limit, T context)
{
    PIXCopyEventArgument(destination, limit, context);
};

#endif //_PIX3_WIN_H_
