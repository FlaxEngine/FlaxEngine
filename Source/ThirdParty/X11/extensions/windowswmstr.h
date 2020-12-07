/*
 * WindowsWM extension is based on AppleWM extension
 * Authors:	Kensuke Matsuzaki
 */
/**************************************************************************

Copyright (c) 2002 Apple Computer, Inc.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#ifndef _WINDOWSWMSTR_H_
#define _WINDOWSWMSTR_H_

#include <X11/extensions/windowswm.h>
#include <X11/X.h>
#include <X11/Xmd.h>

#define WINDOWSWMNAME "Windows-WM"

#define WINDOWS_WM_MAJOR_VERSION	1	/* current version numbers */
#define WINDOWS_WM_MINOR_VERSION	0
#define WINDOWS_WM_PATCH_VERSION	0

typedef struct _WindowsWMQueryVersion {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMQueryVersion */
    CARD16	length B16;
} xWindowsWMQueryVersionReq;
#define sz_xWindowsWMQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	majorVersion B16;	/* major version of WM protocol */
    CARD16	minorVersion B16;	/* minor version of WM protocol */
    CARD32	patchVersion B32;       /* patch version of WM protocol */
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xWindowsWMQueryVersionReply;
#define sz_xWindowsWMQueryVersionReply	32

typedef struct _WindowsWMDisableUpdate {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMDisableUpdate */
    CARD16	length B16;
    CARD32	screen B32;
} xWindowsWMDisableUpdateReq;
#define sz_xWindowsWMDisableUpdateReq	8

typedef struct _WindowsWMReenableUpdate {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMReenableUpdate */
    CARD16	length B16;
    CARD32	screen B32;
} xWindowsWMReenableUpdateReq;
#define sz_xWindowsWMReenableUpdateReq	8

typedef struct _WindowsWMSelectInput {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMSelectInput */
    CARD16	length B16;
    CARD32	mask B32;
} xWindowsWMSelectInputReq;
#define sz_xWindowsWMSelectInputReq	8

typedef struct _WindowsWMNotify {
	BYTE	type;		/* always eventBase + event type */
	BYTE	kind;
	CARD16	sequenceNumber B16;
	Window	window B32;
	Time	time B32;	/* time of change */
	CARD16	pad1 B16;
	CARD32	arg B32;
	INT16	x B16;
	INT16	y B16;
	CARD16	w B16;
	CARD16	h B16;
} xWindowsWMNotifyEvent;
#define sz_xWindowsWMNotifyEvent	28

typedef struct _WindowsWMSetFrontProcess {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMSetFrontProcess */
    CARD16	length B16;
} xWindowsWMSetFrontProcessReq;
#define sz_xWindowsWMSetFrontProcessReq 4

typedef struct _WindowsWMFrameGetRect {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMFrameGetRect */
    CARD16	length B16;
    CARD32	frame_style B32;
    CARD32	frame_style_ex B32;
    CARD16	frame_rect B16;
    INT16	ix B16;
    INT16	iy B16;
    CARD16	iw B16;
    CARD16	ih B16;
    CARD16	pad1 B16;
} xWindowsWMFrameGetRectReq;
#define sz_xWindowsWMFrameGetRectReq	24

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    INT16	x B16;
    INT16	y B16;
    CARD16	w B16;
    CARD16	h B16;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xWindowsWMFrameGetRectReply;
#define sz_xWindowsWMFrameGetRectReply	32

typedef struct _WindowsWMFrameDraw {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMFrameDraw */
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	window B32;
    CARD32	frame_style B32;
    CARD32	frame_style_ex B32;
    INT16	ix B16;
    INT16	iy B16;
    CARD16	iw B16;
    CARD16	ih B16;
} xWindowsWMFrameDrawReq;
#define sz_xWindowsWMFrameDrawReq	28

typedef struct _WindowsWMFrameSetTitle {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_WMFrameSetTitle */
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	window B32;
    CARD32	title_length B32;
} xWindowsWMFrameSetTitleReq;
#define sz_xWindowsWMFrameSetTitleReq	16

#endif /* _WINDOWSWMSTR_H_ */
