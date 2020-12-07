/*
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2010 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _XFIXESPROTO_H_
#define _XFIXESPROTO_H_

#include <X11/Xmd.h>
#include <X11/extensions/xfixeswire.h>
#include <X11/extensions/shapeconst.h>

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
#define Picture CARD32

/*************** Version 1 ******************/

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
} xXFixesReq;

/* 
 * requests and replies
 */
typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    CARD32  majorVersion B32;
    CARD32  minorVersion B32;
} xXFixesQueryVersionReq;

#define sz_xXFixesQueryVersionReq   12

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD32  majorVersion B32;
    CARD32  minorVersion B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
    CARD32  pad4 B32;
    CARD32  pad5 B32;
} xXFixesQueryVersionReply;

#define sz_xXFixesQueryVersionReply	32

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    BYTE    mode;	    /* SetModeInsert/SetModeDelete*/
    BYTE    target;	    /* SaveSetNearest/SaveSetRoot*/
    BYTE    map;	    /* SaveSetMap/SaveSetUnmap */
    BYTE    pad1;
    Window  window;
} xXFixesChangeSaveSetReq;

#define sz_xXFixesChangeSaveSetReq	12

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Window  window  B32;
    Atom    selection B32;
    CARD32  eventMask B32;
} xXFixesSelectSelectionInputReq;

#define sz_xXFixesSelectSelectionInputReq   16

typedef struct {
    CARD8   type;
    CARD8   subtype;
    CARD16  sequenceNumber B16;
    Window  window B32;
    Window  owner B32;
    Atom    selection B32;
    Time    timestamp B32;
    Time    selectionTimestamp B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
} xXFixesSelectionNotifyEvent;

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Window  window B32;
    CARD32  eventMask B32;
} xXFixesSelectCursorInputReq;

#define sz_xXFixesSelectCursorInputReq	12

typedef struct {
    CARD8   type;
    CARD8   subtype;
    CARD16  sequenceNumber B16;
    Window  window B32;
    CARD32  cursorSerial B32;
    Time    timestamp;
    Atom    name B32;	    /* Version 2 */
    CARD32  pad1 B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
} xXFixesCursorNotifyEvent;

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
} xXFixesGetCursorImageReq;

#define sz_xXFixesGetCursorImageReq 4

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    INT16   x B16;
    INT16   y B16;
    CARD16  width B16;
    CARD16  height B16;
    CARD16  xhot B16;
    CARD16  yhot B16;
    CARD32  cursorSerial B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
} xXFixesGetCursorImageReply;

#define sz_xXFixesGetCursorImageReply	32

/*************** Version 2 ******************/

#define Region CARD32

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  region B32;
    /* LISTofRECTANGLE */
} xXFixesCreateRegionReq;

#define sz_xXFixesCreateRegionReq	8

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  region B32;
    Pixmap  bitmap B32;
} xXFixesCreateRegionFromBitmapReq;

#define sz_xXFixesCreateRegionFromBitmapReq	12

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  region B32;
    Window  window B32;
    CARD8   kind;
    CARD8   pad1;
    CARD16  pad2 B16;
} xXFixesCreateRegionFromWindowReq;

#define sz_xXFixesCreateRegionFromWindowReq	16

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  region B32;
    GContext gc B32;
} xXFixesCreateRegionFromGCReq;

#define sz_xXFixesCreateRegionFromGCReq	12

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  region B32;
    Picture picture B32;
} xXFixesCreateRegionFromPictureReq;

#define sz_xXFixesCreateRegionFromPictureReq	12

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  region B32;
} xXFixesDestroyRegionReq;

#define sz_xXFixesDestroyRegionReq	8

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  region B32;
    /* LISTofRECTANGLE */
} xXFixesSetRegionReq;

#define sz_xXFixesSetRegionReq		8

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  source B32;
    Region  destination B32;
} xXFixesCopyRegionReq;

#define sz_xXFixesCopyRegionReq		12

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  source1 B32;
    Region  source2 B32;
    Region  destination B32;
} xXFixesCombineRegionReq,
  xXFixesUnionRegionReq,
  xXFixesIntersectRegionReq,
  xXFixesSubtractRegionReq;

#define sz_xXFixesCombineRegionReq	16
#define sz_xXFixesUnionRegionReq	sz_xXFixesCombineRegionReq
#define sz_xXFixesIntersectRegionReq	sz_xXFixesCombineRegionReq
#define sz_xXFixesSubtractRegionReq	sz_xXFixesCombineRegionReq

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  source B32;
    INT16   x B16, y B16;
    CARD16  width B16, height B16;
    Region  destination B32;
} xXFixesInvertRegionReq;

#define sz_xXFixesInvertRegionReq	20

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  region B32;
    INT16   dx B16, dy B16;
} xXFixesTranslateRegionReq;

#define sz_xXFixesTranslateRegionReq	12
    
typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  source B32;
    Region  destination B32;
} xXFixesRegionExtentsReq;

#define sz_xXFixesRegionExtentsReq	12

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  region B32;
} xXFixesFetchRegionReq;

#define sz_xXFixesFetchRegionReq	8

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    INT16   x B16, y B16;
    CARD16  width B16, height B16;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
    CARD32  pad4 B32;
    CARD32  pad5 B32;
} xXFixesFetchRegionReply;

#define sz_xXFixesFetchRegionReply	32

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    GContext	gc B32;
    Region  region B32;
    INT16   xOrigin B16, yOrigin B16;
} xXFixesSetGCClipRegionReq;

#define sz_xXFixesSetGCClipRegionReq	16

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Window  dest;
    BYTE    destKind;
    CARD8   pad1;
    CARD16  pad2 B16;
    INT16   xOff B16, yOff B16;
    Region  region;
} xXFixesSetWindowShapeRegionReq;

#define sz_xXFixesSetWindowShapeRegionReq	20

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Picture picture B32;
    Region  region B32;
    INT16   xOrigin B16, yOrigin B16;
} xXFixesSetPictureClipRegionReq;

#define sz_xXFixesSetPictureClipRegionReq   16

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Cursor  cursor B32;
    CARD16  nbytes B16;
    CARD16  pad B16;
} xXFixesSetCursorNameReq;

#define sz_xXFixesSetCursorNameReq	    12

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Cursor  cursor B32;
} xXFixesGetCursorNameReq;

#define sz_xXFixesGetCursorNameReq	    8

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    Atom    atom B32;
    CARD16  nbytes B16;
    CARD16  pad2 B16;
    CARD32  pad3 B32;
    CARD32  pad4 B32;
    CARD32  pad5 B32;
    CARD32  pad6 B32;
} xXFixesGetCursorNameReply;

#define sz_xXFixesGetCursorNameReply	    32

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
} xXFixesGetCursorImageAndNameReq;

#define sz_xXFixesGetCursorImageAndNameReq  4

typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    INT16   x B16;
    INT16   y B16;
    CARD16  width B16;
    CARD16  height B16;
    CARD16  xhot B16;
    CARD16  yhot B16;
    CARD32  cursorSerial B32;
    Atom    cursorName B32;
    CARD16  nbytes B16;
    CARD16  pad B16;
} xXFixesGetCursorImageAndNameReply;

#define sz_xXFixesGetCursorImageAndNameReply	32

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Cursor  source B32;
    Cursor  destination B32;
} xXFixesChangeCursorReq;

#define sz_xXFixesChangeCursorReq	12

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Cursor  source B32;
    CARD16  nbytes;
    CARD16  pad;
} xXFixesChangeCursorByNameReq;

#define sz_xXFixesChangeCursorByNameReq	12

/*************** Version 3 ******************/

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Region  source B32;
    Region  destination B32;
    CARD16  left B16;
    CARD16  right B16;
    CARD16  top B16;
    CARD16  bottom B16;
} xXFixesExpandRegionReq;

#define sz_xXFixesExpandRegionReq	20

/*************** Version 4.0 ******************/

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Window  window B32;
} xXFixesHideCursorReq;

#define sz_xXFixesHideCursorReq	sizeof(xXFixesHideCursorReq)

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Window  window B32;
} xXFixesShowCursorReq;

#define sz_xXFixesShowCursorReq	sizeof(xXFixesShowCursorReq)

/*************** Version 5.0 ******************/

#define Barrier CARD32

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Barrier barrier B32;
    Window  window B32;
    INT16   x1 B16;
    INT16   y1 B16;
    INT16   x2 B16;
    INT16   y2 B16;
    CARD32  directions;
    CARD16  pad B16;
    CARD16  num_devices B16;
    /* array of CARD16 devices */
} xXFixesCreatePointerBarrierReq;

#define sz_xXFixesCreatePointerBarrierReq 28

typedef struct {
    CARD8   reqType;
    CARD8   xfixesReqType;
    CARD16  length B16;
    Barrier barrier B32;
} xXFixesDestroyPointerBarrierReq;

#define sz_xXFixesDestroyPointerBarrierReq 8

#undef Barrier
#undef Region
#undef Picture
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

#endif /* _XFIXESPROTO_H_ */
