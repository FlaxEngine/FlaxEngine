/*
 * Copyright © 2003 Keith Packard
 * Copyright © 2007 Eric Anholt
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

#ifndef _DAMAGEPROTO_H_
#define _DAMAGEPROTO_H_

#include <X11/Xmd.h>
#include <X11/extensions/xfixesproto.h>
#include <X11/extensions/damagewire.h>

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
#define Region CARD32
#define Damage CARD32

/************** Version 0 ******************/

typedef struct {
    CARD8   reqType;
    CARD8   damageReqType;
    CARD16  length B16;
} xDamageReq;

/* 
 * requests and replies
 */

typedef struct {
    CARD8   reqType;
    CARD8   damageReqType;
    CARD16  length B16;
    CARD32  majorVersion B32;
    CARD32  minorVersion B32;
} xDamageQueryVersionReq;

#define sz_xDamageQueryVersionReq   12

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
} xDamageQueryVersionReply;

#define sz_xDamageQueryVersionReply	32

typedef struct {
    CARD8	reqType;
    CARD8	damageReqType;
    CARD16	length B16;
    Damage	damage B32;
    Drawable	drawable B32;
    CARD8	level;
    CARD8	pad1;
    CARD16	pad2 B16;
} xDamageCreateReq;

#define sz_xDamageCreateReq		16

typedef struct {
    CARD8	reqType;
    CARD8	damageReqType;
    CARD16	length B16;
    Damage	damage B32;
} xDamageDestroyReq;

#define sz_xDamageDestroyReq		8

typedef struct {
    CARD8	reqType;
    CARD8	damageReqType;
    CARD16	length B16;
    Damage	damage B32;
    Region	repair B32;
    Region	parts B32;
} xDamageSubtractReq;

#define sz_xDamageSubtractReq		16

typedef struct {
    CARD8	reqType;
    CARD8	damageReqType;
    CARD16	length B16;
    Drawable	drawable B32;
    Region	region B32;
} xDamageAddReq;

#define sz_xDamageAddReq		12

/* Events */

#define DamageNotifyMore    0x80

typedef struct {
    CARD8	type;
    CARD8	level;
    CARD16	sequenceNumber B16;
    Drawable	drawable B32;
    Damage	damage B32;
    Time	timestamp B32;
    xRectangle	area;
    xRectangle	geometry;
} xDamageNotifyEvent;

#undef Damage
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

#endif /* _DAMAGEPROTO_H_ */
