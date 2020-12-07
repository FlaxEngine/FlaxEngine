/*
 * Copyright © 2013 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef _DRI3_PROTO_H_
#define _DRI3_PROTO_H_

#define DRI3_NAME			"DRI3"
#define DRI3_MAJOR			1
#define DRI3_MINOR			2

#define DRI3NumberErrors		0
#define DRI3NumberEvents		0

#define X_DRI3QueryVersion		0
#define X_DRI3Open			1
#define X_DRI3PixmapFromBuffer          2
#define X_DRI3BufferFromPixmap          3
#define X_DRI3FenceFromFD               4
#define X_DRI3FDFromFence               5

/* v1.2 */
#define xDRI3GetSupportedModifiers      6
#define xDRI3PixmapFromBuffers          7
#define xDRI3BuffersFromPixmap          8

#define DRI3NumberRequests		9

typedef struct {
    CARD8   reqType;
    CARD8   dri3ReqType;
    CARD16  length B16;
    CARD32  majorVersion B32;
    CARD32  minorVersion B32;
} xDRI3QueryVersionReq;
#define sz_xDRI3QueryVersionReq   12

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
} xDRI3QueryVersionReply;
#define sz_xDRI3QueryVersionReply	32

typedef struct {
    CARD8   reqType;
    CARD8   dri3ReqType;
    CARD16  length B16;
    CARD32  drawable B32;
    CARD32  provider B32;
} xDRI3OpenReq;
#define sz_xDRI3OpenReq	12

typedef struct {
    BYTE    type;   /* X_Reply */
    CARD8   nfd;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
    CARD32  pad4 B32;
    CARD32  pad5 B32;
    CARD32  pad6 B32;
    CARD32  pad7 B32;
} xDRI3OpenReply;
#define sz_xDRI3OpenReply	32

typedef struct {
    CARD8   reqType;
    CARD8   dri3ReqType;
    CARD16  length B16;
    CARD32  pixmap B32;
    CARD32  drawable B32;
    CARD32  size B32;
    CARD16  width B16;
    CARD16  height B16;
    CARD16  stride B16;
    CARD8   depth;
    CARD8   bpp;
} xDRI3PixmapFromBufferReq;

#define sz_xDRI3PixmapFromBufferReq     24

typedef struct {
    CARD8   reqType;
    CARD8   dri3ReqType;
    CARD16  length B16;
    CARD32  pixmap B32;
} xDRI3BufferFromPixmapReq;
#define sz_xDRI3BufferFromPixmapReq     8

typedef struct {
    BYTE    type;   /* X_Reply */
    CARD8   nfd;    /* Number of file descriptors returned (1) */
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD32  size B32;
    CARD16  width B16;
    CARD16  height B16;
    CARD16  stride B16;
    CARD8   depth;
    CARD8   bpp;
    CARD32  pad20 B32;
    CARD32  pad24 B32;
    CARD32  pad28 B32;
} xDRI3BufferFromPixmapReply;
#define sz_xDRI3BufferFromPixmapReply   32

typedef struct {
    CARD8   reqType;
    CARD8   dri3ReqType;
    CARD16  length B16;
    CARD32  drawable B32;
    CARD32  fence B32;
    BOOL    initially_triggered;
    CARD8   pad13;
    CARD16  pad14 B16;
} xDRI3FenceFromFDReq;

#define sz_xDRI3FenceFromFDReq  16

typedef struct {
    CARD8   reqType;
    CARD8   dri3ReqType;
    CARD16  length B16;
    CARD32  drawable B32;
    CARD32  fence B32;
} xDRI3FDFromFenceReq;

#define sz_xDRI3FDFromFenceReq  12

typedef struct {
    BYTE    type;   /* X_Reply */
    CARD8   nfd;    /* Number of file descriptors returned (1) */
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD32  pad08 B32;
    CARD32  pad12 B32;
    CARD32  pad16 B32;
    CARD32  pad20 B32;
    CARD32  pad24 B32;
    CARD32  pad28 B32;
} xDRI3FDFromFenceReply;

#define sz_xDRI3FDFromFenceReply   32

/* v1.2 */

typedef struct {
    CARD8   reqType;
    CARD8   dri3ReqType;
    CARD16  length B16;
    CARD32  window B32;
    CARD8   depth;
    CARD8   bpp;
    CARD16  pad10 B16;
} xDRI3GetSupportedModifiersReq;
#define sz_xDRI3GetSupportedModifiersReq     12

typedef struct {
    BYTE    type;   /* X_Reply */
    CARD8   pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD32  numWindowModifiers B32;
    CARD32  numScreenModifiers B32;
    CARD32  pad16 B32;
    CARD32  pad20 B32;
    CARD32  pad24 B32;
    CARD32  pad28 B32;
} xDRI3GetSupportedModifiersReply;
#define sz_xDRI3GetSupportedModifiersReply   32

typedef struct {
    CARD8   reqType;
    CARD8   dri3ReqType;
    CARD16  length B16;
    CARD32  pixmap B32;
    CARD32  window B32;
    CARD8   num_buffers; /* Number of file descriptors passed */
    CARD8   pad13;
    CARD16  pad14 B16;
    CARD16  width B16;
    CARD16  height B16;
    CARD32  stride0 B32;
    CARD32  offset0 B32;
    CARD32  stride1 B32;
    CARD32  offset1 B32;
    CARD32  stride2 B32;
    CARD32  offset2 B32;
    CARD32  stride3 B32;
    CARD32  offset3 B32;
    CARD8   depth;
    CARD8   bpp;
    CARD16  pad54 B16;
    CARD64  modifier;
} xDRI3PixmapFromBuffersReq;
#define sz_xDRI3PixmapFromBuffersReq 64

typedef struct {
    CARD8   reqType;
    CARD8   dri3ReqType;
    CARD16  length B16;
    CARD32  pixmap B32;
} xDRI3BuffersFromPixmapReq;
#define sz_xDRI3BuffersFromPixmapReq     8

typedef struct {
    BYTE    type;   /* X_Reply */
    CARD8   nfd;    /* Number of file descriptors returned */
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD16  width B16;
    CARD16  height B16;
    CARD32  pad12 B32;
    CARD64  modifier;
    CARD8   depth;
    CARD8   bpp;
    CARD16  pad26 B16;
    CARD32  pad28 B32;
} xDRI3BuffersFromPixmapReply;
#define sz_xDRI3BuffersFromPixmapReply   32

#endif
