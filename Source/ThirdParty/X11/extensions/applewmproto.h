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

#ifndef _APPLEWMPROTO_H_
#define _APPLEWMPROTO_H_

#include <X11/extensions/applewmconst.h>
#include <X11/X.h>
#include <X11/Xmd.h>

#define APPLEWMNAME "Apple-WM"

#define APPLE_WM_MAJOR_VERSION	1	/* current version numbers */
#define APPLE_WM_MINOR_VERSION	3
#define APPLE_WM_PATCH_VERSION	0

#define X_AppleWMQueryVersion		0
#define X_AppleWMFrameGetRect		1
#define X_AppleWMFrameHitTest		2
#define X_AppleWMFrameDraw		3
#define X_AppleWMDisableUpdate		4
#define X_AppleWMReenableUpdate		5
#define X_AppleWMSelectInput		6
#define X_AppleWMSetWindowMenuCheck	7
#define X_AppleWMSetFrontProcess	8
#define X_AppleWMSetWindowLevel		9
#define X_AppleWMSetCanQuit		10
#define X_AppleWMSetWindowMenu		11
#define X_AppleWMSendPSN		12
#define X_AppleWMAttachTransient	13

/* For the purpose of the structure definitions in this file,
we must redefine the following types in terms of Xmd.h's types, which may
include bit fields.  All of these are #undef'd at the end of this file,
restoring the definitions in X.h.  */

#define Window CARD32
#define Drawable CARD32
#define Font CARD32
#define Pixmap CARD32
#define Cursor CARD32
#define Colormap CARD32
#define GContext CARD32
#define Atom CARD32
#define VisualID CARD32
#define Time CARD32
#define KeyCode CARD8
#define KeySym CARD32

typedef struct _AppleWMQueryVersion {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMQueryVersion */
    CARD16	length B16;
} xAppleWMQueryVersionReq;
#define sz_xAppleWMQueryVersionReq	4

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
} xAppleWMQueryVersionReply;
#define sz_xAppleWMQueryVersionReply	32

typedef struct _AppleWMDisableUpdate {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMDisableUpdate */
    CARD16	length B16;
    CARD32	screen B32;
} xAppleWMDisableUpdateReq;
#define sz_xAppleWMDisableUpdateReq	8

typedef struct _AppleWMReenableUpdate {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMReenableUpdate */
    CARD16	length B16;
    CARD32	screen B32;
} xAppleWMReenableUpdateReq;
#define sz_xAppleWMReenableUpdateReq	8

typedef struct _AppleWMSelectInput {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMSelectInput */
    CARD16	length B16;
    CARD32	mask B32;
} xAppleWMSelectInputReq;
#define sz_xAppleWMSelectInputReq	8

typedef struct _AppleWMNotify {
	BYTE	type;		/* always eventBase + event type */
	BYTE	kind;
	CARD16	sequenceNumber B16;
	Time	time B32;	/* time of change */
	CARD16	pad1 B16;
	CARD32	arg B32;
	CARD32	pad3 B32;
	CARD32  pad4 B32;
	CARD32  pad5 B32;
	CARD32  pad6 B32;
} xAppleWMNotifyEvent;
#define sz_xAppleWMNotifyEvent	32

typedef struct _AppleWMSetWindowMenu {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMSetWindowMenu */
    CARD16	length B16;
    CARD16	nitems B16;
    CARD16	pad1 B16;
} xAppleWMSetWindowMenuReq;
#define sz_xAppleWMSetWindowMenuReq	8

typedef struct _AppleWMSetWindowMenuCheck {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMSetWindowMenuCheck */
    CARD16	length B16;
    CARD32	index;
} xAppleWMSetWindowMenuCheckReq;
#define sz_xAppleWMSetWindowMenuCheckReq 8

typedef struct _AppleWMSetFrontProcess {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMSetFrontProcess */
    CARD16	length B16;
} xAppleWMSetFrontProcessReq;
#define sz_xAppleWMSetFrontProcessReq 4

typedef struct _AppleWMSetWindowLevel {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMSetWindowLevel */
    CARD16	length B16;
    CARD32	window;
    CARD32	level;
} xAppleWMSetWindowLevelReq;
#define sz_xAppleWMSetWindowLevelReq 12

typedef struct _AppleWMSendPSN {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMSendPSN */
    CARD16	length B16;
    CARD32	psn_hi;
    CARD32	psn_lo;
} xAppleWMSendPSNReq;
#define sz_xAppleWMSendPSNReq 12

typedef struct _AppleWMAttachTransient {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMAttachTransient */
    CARD16	length B16;
    CARD32	child;
    CARD32	parent;
} xAppleWMAttachTransientReq;
#define sz_xAppleWMAttachTransientReq 12

typedef struct _AppleWMSetCanQuit {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMSetCanQuit */
    CARD16	length B16;
    CARD32	state;
} xAppleWMSetCanQuitReq;
#define sz_xAppleWMSetCanQuitReq 8

typedef struct _AppleWMFrameGetRect {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMFrameGetRect */
    CARD16	length B16;
    CARD16	frame_class B16;
    CARD16	frame_rect B16;
    CARD16	ix B16;
    CARD16	iy B16;
    CARD16	iw B16;
    CARD16	ih B16;
    CARD16	ox B16;
    CARD16	oy B16;
    CARD16	ow B16;
    CARD16	oh B16;
} xAppleWMFrameGetRectReq;
#define sz_xAppleWMFrameGetRectReq	24

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	x B16;
    CARD16	y B16;
    CARD16	w B16;
    CARD16	h B16;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xAppleWMFrameGetRectReply;
#define sz_xAppleWMFrameGetRectReply	32

typedef struct _AppleWMFrameHitTest {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMFrameHitTest */
    CARD16	length B16;
    CARD16	frame_class B16;
    CARD16	pad1 B16;
    CARD16	px B16;
    CARD16	py B16;
    CARD16	ix B16;
    CARD16	iy B16;
    CARD16	iw B16;
    CARD16	ih B16;
    CARD16	ox B16;
    CARD16	oy B16;
    CARD16	ow B16;
    CARD16	oh B16;
} xAppleWMFrameHitTestReq;
#define sz_xAppleWMFrameHitTestReq	28

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	ret B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xAppleWMFrameHitTestReply;
#define sz_xAppleWMFrameHitTestReply	32

typedef struct _AppleWMFrameDraw {
    CARD8	reqType;		/* always WMReqCode */
    CARD8	wmReqType;		/* always X_AppleWMFrameDraw */
    CARD16	length B16;
    CARD32	screen B32;
    CARD32	window B32;
    CARD16	frame_class B16;
    CARD16	frame_attr B16;
    CARD16	ix B16;
    CARD16	iy B16;
    CARD16	iw B16;
    CARD16	ih B16;
    CARD16	ox B16;
    CARD16	oy B16;
    CARD16	ow B16;
    CARD16	oh B16;
    CARD32	title_length B32;
} xAppleWMFrameDrawReq;
#define sz_xAppleWMFrameDrawReq	36

/* restore these definitions back to the typedefs in X.h */
#undef Window
#undef Drawable
#undef Font
#undef Pixmap
#undef Cursor
#undef Colormap
#undef GContext
#undef Atom
#undef VisualID
#undef Time
#undef KeyCode
#undef KeySym

#endif /* _APPLEWMPROTO_H_ */
