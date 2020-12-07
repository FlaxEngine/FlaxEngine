/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
 * Copyright © 2008 Red Hat, Inc.
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
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

/* note that RANDR 1.0 is incompatible with version 0.0, or 0.1 */
/* V1.0 removes depth switching from the protocol */
#ifndef _XRANDRP_H_
#define _XRANDRP_H_

#include <X11/extensions/randr.h>
#include <X11/extensions/renderproto.h>

#define Window CARD32
#define Drawable CARD32
#define Font CARD32
#define Pixmap CARD32
#define Cursor CARD32
#define Colormap CARD32
#define GContext CARD32
#define Atom CARD32
#define Time CARD32
#define KeyCode CARD8
#define KeySym CARD32
#define RROutput CARD32
#define RRMode CARD32
#define RRCrtc CARD32
#define RRProvider CARD32
#define RRModeFlags CARD32
#define RRLease CARD32

#define Rotation CARD16
#define SizeID CARD16
#define SubpixelOrder CARD16

/*
 * data structures
 */

typedef struct {
    CARD16 widthInPixels B16;
    CARD16 heightInPixels B16;
    CARD16 widthInMillimeters B16;
    CARD16 heightInMillimeters B16;
} xScreenSizes;
#define sz_xScreenSizes 8

/* 
 * requests and replies
 */

typedef struct {
    CARD8   reqType;
    CARD8   randrReqType;
    CARD16  length B16;
    CARD32  majorVersion B32;
    CARD32  minorVersion B32;
} xRRQueryVersionReq;
#define sz_xRRQueryVersionReq   12

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
} xRRQueryVersionReply;
#define sz_xRRQueryVersionReply	32

typedef struct {
    CARD8   reqType;
    CARD8   randrReqType;
    CARD16  length B16;
    Window  window B32;
} xRRGetScreenInfoReq;
#define sz_xRRGetScreenInfoReq   8

/* 
 * the xRRScreenInfoReply structure is followed by:
 *
 * the size information
 */


typedef struct {
    BYTE    type;   /* X_Reply */
    BYTE    setOfRotations;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    Window  root B32;
    Time    timestamp B32;
    Time    configTimestamp B32;
    CARD16  nSizes B16;
    SizeID  sizeID B16;
    Rotation  rotation B16;
    CARD16  rate B16;
    CARD16  nrateEnts B16;
    CARD16  pad B16;
} xRRGetScreenInfoReply;
#define sz_xRRGetScreenInfoReply	32

typedef struct {
    CARD8    reqType;
    CARD8    randrReqType;
    CARD16   length B16;
    Drawable drawable B32;
    Time     timestamp B32;
    Time     configTimestamp B32;
    SizeID   sizeID B16;
    Rotation rotation B16;
} xRR1_0SetScreenConfigReq;
#define sz_xRR1_0SetScreenConfigReq   20

typedef struct {
    CARD8    reqType;
    CARD8    randrReqType;
    CARD16   length B16;
    Drawable drawable B32;
    Time     timestamp B32;
    Time     configTimestamp B32;
    SizeID   sizeID B16;
    Rotation rotation B16;
    CARD16   rate B16;
    CARD16   pad B16;
} xRRSetScreenConfigReq;
#define sz_xRRSetScreenConfigReq   24

typedef struct {
    BYTE    type;   /* X_Reply */
    CARD8   status;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    Time    newTimestamp B32;  
    Time    newConfigTimestamp B32;
    Window  root;
    CARD16  subpixelOrder B16;
    CARD16  pad4 B16;
    CARD32  pad5 B32;
    CARD32  pad6 B32;
} xRRSetScreenConfigReply;
#define sz_xRRSetScreenConfigReply 32

typedef struct {
    CARD8   reqType;
    CARD8   randrReqType;
    CARD16  length B16;
    Window  window B32;
    CARD16  enable B16;
    CARD16  pad2 B16;
} xRRSelectInputReq;
#define sz_xRRSelectInputReq   12

/*
 * Additions for version 1.2
 */

typedef struct _xRRModeInfo {
    RRMode		id B32;
    CARD16		width B16;
    CARD16		height B16;
    CARD32		dotClock B32;
    CARD16		hSyncStart B16;
    CARD16		hSyncEnd B16;
    CARD16		hTotal B16;
    CARD16		hSkew B16;
    CARD16		vSyncStart B16;
    CARD16		vSyncEnd B16;
    CARD16		vTotal B16;
    CARD16		nameLength B16;
    RRModeFlags		modeFlags B32;
} xRRModeInfo;
#define sz_xRRModeInfo		    32

typedef struct {
    CARD8   reqType;
    CARD8   randrReqType;
    CARD16  length B16;
    Window  window B32;
} xRRGetScreenSizeRangeReq;
#define sz_xRRGetScreenSizeRangeReq 8

typedef struct {
    BYTE    type;   /* X_Reply */
    CARD8   pad;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD16  minWidth B16;
    CARD16  minHeight B16;
    CARD16  maxWidth B16;
    CARD16  maxHeight B16;
    CARD32  pad0 B32;
    CARD32  pad1 B32;
    CARD32  pad2 B32;
    CARD32  pad3 B32;
} xRRGetScreenSizeRangeReply;
#define sz_xRRGetScreenSizeRangeReply 32

typedef struct {
    CARD8   reqType;
    CARD8   randrReqType;
    CARD16  length B16;
    Window  window B32;
    CARD16  width B16;
    CARD16  height B16;
    CARD32  widthInMillimeters B32;
    CARD32  heightInMillimeters B32;
} xRRSetScreenSizeReq;
#define sz_xRRSetScreenSizeReq	    20

typedef struct {
    CARD8   reqType;
    CARD8   randrReqType;
    CARD16  length B16;
    Window  window B32;
} xRRGetScreenResourcesReq;
#define sz_xRRGetScreenResourcesReq 8

typedef struct {
    BYTE	type;
    CARD8	pad;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Time	timestamp B32;
    Time	configTimestamp B32;
    CARD16	nCrtcs B16;
    CARD16	nOutputs B16;
    CARD16	nModes B16;
    CARD16	nbytesNames B16;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
} xRRGetScreenResourcesReply;
#define sz_xRRGetScreenResourcesReply	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RROutput	output B32;
    Time	configTimestamp B32;
} xRRGetOutputInfoReq;
#define sz_xRRGetOutputInfoReq		12

typedef struct {
    BYTE	type;
    CARD8	status;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Time	timestamp B32;
    RRCrtc	crtc B32;
    CARD32	mmWidth B32;
    CARD32	mmHeight B32;
    CARD8	connection;
    CARD8	subpixelOrder;
    CARD16	nCrtcs B16;
    CARD16	nModes B16;
    CARD16	nPreferred B16;
    CARD16	nClones B16;
    CARD16	nameLength B16;
} xRRGetOutputInfoReply;
#define sz_xRRGetOutputInfoReply	36

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RROutput	output B32;
} xRRListOutputPropertiesReq; 
#define sz_xRRListOutputPropertiesReq	8

typedef struct {
    BYTE	type;
    CARD8	pad0;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	nAtoms B16;
    CARD16	pad1 B16;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xRRListOutputPropertiesReply;
#define sz_xRRListOutputPropertiesReply	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RROutput	output B32;
    Atom	property B32;
} xRRQueryOutputPropertyReq; 
#define sz_xRRQueryOutputPropertyReq	12

typedef struct {
    BYTE	type;
    BYTE	pad0;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    BOOL	pending;
    BOOL	range;
    BOOL	immutable;
    BYTE	pad1;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xRRQueryOutputPropertyReply;
#define sz_xRRQueryOutputPropertyReply	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RROutput	output B32;
    Atom	property B32;
    BOOL	pending;
    BOOL	range;
    CARD16	pad B16;
} xRRConfigureOutputPropertyReq; 
#define sz_xRRConfigureOutputPropertyReq	16

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RROutput	output B32;
    Atom	property B32;
    Atom	type B32;
    CARD8	format;
    CARD8	mode;
    CARD16	pad;
    CARD32	nUnits B32;
} xRRChangeOutputPropertyReq;
#define sz_xRRChangeOutputPropertyReq	24

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RROutput	output B32;
    Atom	property B32;
} xRRDeleteOutputPropertyReq;
#define sz_xRRDeleteOutputPropertyReq	12

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RROutput	output B32;
    Atom	property B32;
    Atom	type B32;
    CARD32	longOffset B32;
    CARD32	longLength B32;
#ifdef __cplusplus
    BOOL	_delete;
#else
    BOOL	delete;
#endif
    BOOL	pending;
    CARD16	pad1 B16;
} xRRGetOutputPropertyReq;
#define sz_xRRGetOutputPropertyReq	28

typedef struct {
    BYTE	type;
    CARD8	format;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Atom	propertyType B32;
    CARD32	bytesAfter B32;
    CARD32	nItems B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
} xRRGetOutputPropertyReply;
#define sz_xRRGetOutputPropertyReply	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    Window	window B32;
    xRRModeInfo	modeInfo;
} xRRCreateModeReq; 
#define sz_xRRCreateModeReq		40

typedef struct {
    BYTE	type;
    CARD8	pad0;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    RRMode	mode B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xRRCreateModeReply;
#define sz_xRRCreateModeReply		32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRMode	mode B32;
} xRRDestroyModeReq;
#define sz_xRRDestroyModeReq		8

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RROutput	output B32;
    RRMode	mode B32;
} xRRAddOutputModeReq;
#define sz_xRRAddOutputModeReq		12

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RROutput	output B32;
    RRMode	mode B32;
} xRRDeleteOutputModeReq;
#define sz_xRRDeleteOutputModeReq	12

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRCrtc	crtc B32;
    Time	configTimestamp B32;
} xRRGetCrtcInfoReq; 
#define sz_xRRGetCrtcInfoReq		12

typedef struct {
    BYTE	type;
    CARD8	status;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Time	timestamp B32;
    INT16	x B16;
    INT16	y B16;
    CARD16	width B16;
    CARD16	height B16;
    RRMode	mode B32;
    Rotation	rotation B16;
    Rotation	rotations B16;
    CARD16	nOutput B16;
    CARD16	nPossibleOutput B16;
} xRRGetCrtcInfoReply;
#define sz_xRRGetCrtcInfoReply		32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRCrtc	crtc B32;
    Time	timestamp B32;
    Time    	configTimestamp B32;
    INT16	x B16;
    INT16	y B16;
    RRMode	mode B32;
    Rotation	rotation B16;
    CARD16	pad B16;
} xRRSetCrtcConfigReq; 
#define sz_xRRSetCrtcConfigReq		28

typedef struct {
    BYTE	type;
    CARD8	status;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Time	newTimestamp B32;
    CARD32	pad1 B32;
    CARD32	pad2 B16;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xRRSetCrtcConfigReply;
#define sz_xRRSetCrtcConfigReply	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRCrtc	crtc B32;
} xRRGetCrtcGammaSizeReq; 
#define sz_xRRGetCrtcGammaSizeReq	8

typedef struct {
    BYTE	type;
    CARD8	status;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	size B16;
    CARD16	pad1 B16;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xRRGetCrtcGammaSizeReply;
#define sz_xRRGetCrtcGammaSizeReply	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRCrtc	crtc B32;
} xRRGetCrtcGammaReq; 
#define sz_xRRGetCrtcGammaReq		8

typedef struct {
    BYTE	type;
    CARD8	status;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	size B16;
    CARD16	pad1 B16;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xRRGetCrtcGammaReply;
#define sz_xRRGetCrtcGammaReply		32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRCrtc	crtc B32;
    CARD16	size B16;
    CARD16	pad1 B16;
} xRRSetCrtcGammaReq;
#define sz_xRRSetCrtcGammaReq		12

/*
 * Additions for V1.3
 */

typedef xRRGetScreenResourcesReq xRRGetScreenResourcesCurrentReq;

#define sz_xRRGetScreenResourcesCurrentReq sz_xRRGetScreenResourcesReq

typedef xRRGetScreenResourcesReply xRRGetScreenResourcesCurrentReply;
#define sz_xRRGetScreenResourcesCurrentReply	sz_xRRGetScreenResourcesReply

typedef struct {
    CARD8		reqType;
    CARD8		randrReqType;
    CARD16		length B16;
    RRCrtc		crtc B32;
    xRenderTransform	transform;
    CARD16		nbytesFilter;	/* number of bytes in filter name */
    CARD16		pad B16;
} xRRSetCrtcTransformReq;

#define sz_xRRSetCrtcTransformReq	48

typedef struct {
    CARD8		reqType;
    CARD8		randrReqType;
    CARD16		length B16;
    RRCrtc		crtc B32;
} xRRGetCrtcTransformReq;

#define sz_xRRGetCrtcTransformReq	8

typedef struct {
    BYTE		type;
    CARD8		status;
    CARD16		sequenceNumber B16;
    CARD32		length B32;
    xRenderTransform	pendingTransform;
    BYTE		hasTransforms;
    CARD8		pad0;
    CARD16		pad1 B16;
    xRenderTransform	currentTransform;
    CARD32		pad2 B32;
    CARD16		pendingNbytesFilter B16;    /* number of bytes in filter name */
    CARD16		pendingNparamsFilter B16;   /* number of filter params */
    CARD16		currentNbytesFilter B16;    /* number of bytes in filter name */
    CARD16		currentNparamsFilter B16;   /* number of filter params */
} xRRGetCrtcTransformReply;

#define sz_xRRGetCrtcTransformReply	96

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    Window	window B32;
    RROutput	output B32;
} xRRSetOutputPrimaryReq;
#define sz_xRRSetOutputPrimaryReq	12

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    Window	window B32;
} xRRGetOutputPrimaryReq;
#define sz_xRRGetOutputPrimaryReq	8

typedef struct {
    BYTE	type;
    CARD8	pad;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    RROutput	output B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xRRGetOutputPrimaryReply;
#define sz_xRRGetOutputPrimaryReply	32

/*
 * Additions for V1.4
 */

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    Window	window B32;
} xRRGetProvidersReq;
#define sz_xRRGetProvidersReq 8

typedef struct {
    BYTE	type;
    CARD8	pad;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Time	timestamp B32;
    CARD16	nProviders;
    CARD16	pad1 B16;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xRRGetProvidersReply;
#define sz_xRRGetProvidersReply 32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRProvider	provider B32;
    Time	configTimestamp B32;
} xRRGetProviderInfoReq;
#define sz_xRRGetProviderInfoReq 12

typedef struct {
    BYTE	type;
    CARD8	status;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Time	timestamp B32;
    CARD32	capabilities B32;
    CARD16	nCrtcs B16;
    CARD16	nOutputs B16;
    CARD16	nAssociatedProviders B16;
    CARD16	nameLength B16;
    CARD32      pad1 B32;
    CARD32      pad2 B32;
} xRRGetProviderInfoReply;
#define sz_xRRGetProviderInfoReply 32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRProvider  provider B32;
    RRProvider  source_provider B32;
    Time	configTimestamp B32;
} xRRSetProviderOutputSourceReq;
#define sz_xRRSetProviderOutputSourceReq 16

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRProvider  provider B32;
    RRProvider  sink_provider B32;
    Time	configTimestamp B32;
} xRRSetProviderOffloadSinkReq;
#define sz_xRRSetProviderOffloadSinkReq 16

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRProvider	provider B32;
} xRRListProviderPropertiesReq; 
#define sz_xRRListProviderPropertiesReq	8

typedef struct {
    BYTE	type;
    CARD8	pad0;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	nAtoms B16;
    CARD16	pad1 B16;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xRRListProviderPropertiesReply;
#define sz_xRRListProviderPropertiesReply	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRProvider	provider B32;
    Atom	property B32;
} xRRQueryProviderPropertyReq; 
#define sz_xRRQueryProviderPropertyReq	12

typedef struct {
    BYTE	type;
    BYTE	pad0;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    BOOL	pending;
    BOOL	range;
    BOOL	immutable;
    BYTE	pad1;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xRRQueryProviderPropertyReply;
#define sz_xRRQueryProviderPropertyReply	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRProvider	provider B32;
    Atom	property B32;
    BOOL	pending;
    BOOL	range;
    CARD16	pad B16;
} xRRConfigureProviderPropertyReq; 
#define sz_xRRConfigureProviderPropertyReq	16

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRProvider	provider B32;
    Atom	property B32;
    Atom	type B32;
    CARD8	format;
    CARD8	mode;
    CARD16	pad;
    CARD32	nUnits B32;
} xRRChangeProviderPropertyReq;
#define sz_xRRChangeProviderPropertyReq	24

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRProvider	provider B32;
    Atom	property B32;
} xRRDeleteProviderPropertyReq;
#define sz_xRRDeleteProviderPropertyReq	12

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRProvider	provider B32;
    Atom	property B32;
    Atom	type B32;
    CARD32	longOffset B32;
    CARD32	longLength B32;
#ifdef __cplusplus
    BOOL	_delete;
#else
    BOOL	delete;
#endif
    BOOL	pending;
    CARD16	pad1 B16;
} xRRGetProviderPropertyReq;
#define sz_xRRGetProviderPropertyReq	28

typedef struct {
    BYTE	type;
    CARD8	format;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Atom	propertyType B32;
    CARD32	bytesAfter B32;
    CARD32	nItems B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
} xRRGetProviderPropertyReply;
#define sz_xRRGetProviderPropertyReply	32

/*
 * Additions for V1.6
 */

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    Window	window B32;
    RRLease	lid B32;
    CARD16	nCrtcs B16;
    CARD16	nOutputs B16;
} xRRCreateLeaseReq;
#define sz_xRRCreateLeaseReq	16

typedef struct {
    BYTE	type;
    CARD8	nfd;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
    CARD32	pad7 B32;
} xRRCreateLeaseReply;
#define sz_xRRCreateLeaseReply		32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRLease	lid B32;
    BYTE	terminate;
    CARD8	pad1;
    CARD16	pad2 B16;
} xRRFreeLeaseReq;
#define sz_xRRFreeLeaseReq		12

/*
 * event
 */
typedef struct {
    CARD8 type;				/* always evBase + ScreenChangeNotify */
    CARD8 rotation;			/* new rotation */
    CARD16 sequenceNumber B16;
    Time timestamp B32;			/* time screen was changed */
    Time configTimestamp B32;		/* time config data was changed */
    Window root B32;			/* root window */
    Window window B32;			/* window requesting notification */
    SizeID sizeID B16;			/* new size ID */
    CARD16 subpixelOrder B16;		/* subpixel order */
    CARD16 widthInPixels B16;		/* new size */
    CARD16 heightInPixels B16;
    CARD16 widthInMillimeters B16;
    CARD16 heightInMillimeters B16;
} xRRScreenChangeNotifyEvent;
#define sz_xRRScreenChangeNotifyEvent	32

typedef struct {
    CARD8 type;				/* always evBase + RRNotify */
    CARD8 subCode;			/* RRNotify_CrtcChange */
    CARD16 sequenceNumber B16;
    Time timestamp B32;			/* time crtc was changed */
    Window window B32;			/* window requesting notification */
    RRCrtc crtc B32;			/* affected CRTC */
    RRMode mode B32;			/* current mode */
    CARD16 rotation B16;		/* rotation and reflection */
    CARD16 pad1 B16;			/* unused */
    INT16 x B16;			/* new location */
    INT16 y B16;
    CARD16 width B16;			/* new size */
    CARD16 height B16;
} xRRCrtcChangeNotifyEvent;
#define sz_xRRCrtcChangeNotifyEvent	32

typedef struct {
    CARD8 type;				/* always evBase + RRNotify */
    CARD8 subCode;			/* RRNotify_OutputChange */
    CARD16 sequenceNumber B16;
    Time timestamp B32;			/* time output was changed */
    Time configTimestamp B32;		/* time config was changed */
    Window window B32;			/* window requesting notification */
    RROutput output B32;		/* affected output */
    RRCrtc crtc B32;			/* current crtc */
    RRMode mode B32;			/* current mode */
    CARD16 rotation B16;		/* rotation and reflection */
    CARD8 connection;			/* connection status */
    CARD8 subpixelOrder;		/* subpixel order */
} xRROutputChangeNotifyEvent;
#define sz_xRROutputChangeNotifyEvent	32

typedef struct {
    CARD8 type;				/* always evBase + RRNotify */
    CARD8 subCode;			/* RRNotify_OutputProperty */
    CARD16 sequenceNumber B16;
    Window window B32;			/* window requesting notification */
    RROutput output B32;		/* affected output */
    Atom atom B32;			/* property name */
    Time timestamp B32;			/* time crtc was changed */
    CARD8 state;			/* NewValue or Deleted */
    CARD8 pad1;
    CARD16 pad2 B16;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
} xRROutputPropertyNotifyEvent;
#define sz_xRROutputPropertyNotifyEvent	32

typedef struct {
    CARD8 type;				/* always evBase + RRNotify */
    CARD8 subCode;			/* RRNotify_ProviderChange */
    CARD16 sequenceNumber B16;
    Time timestamp B32;			/* time provider was changed */
    Window window B32;			/* window requesting notification */
    RRProvider provider B32;		/* affected provider */
    CARD32 pad1 B32;
    CARD32 pad2 B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
} xRRProviderChangeNotifyEvent;
#define sz_xRRProviderChangeNotifyEvent	32

typedef struct {
    CARD8 type;				/* always evBase + RRNotify */
    CARD8 subCode;			/* RRNotify_ProviderProperty */
    CARD16 sequenceNumber B16;
    Window window B32;			/* window requesting notification */
    RRProvider provider B32;		/* affected provider */
    Atom atom B32;			/* property name */
    Time timestamp B32;			/* time provider was changed */
    CARD8 state;			/* NewValue or Deleted */
    CARD8 pad1;
    CARD16 pad2 B16;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
} xRRProviderPropertyNotifyEvent;
#define sz_xRRProviderPropertyNotifyEvent	32

typedef struct {
    CARD8 type;				/* always evBase + RRNotify */
    CARD8 subCode;			/* RRNotify_ResourceChange */
    CARD16 sequenceNumber B16;
    Time timestamp B32;			/* time resource was changed */
    Window window B32;			/* window requesting notification */
    CARD32 pad1 B32;
    CARD32 pad2 B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
} xRRResourceChangeNotifyEvent;
#define sz_xRRResourceChangeNotifyEvent	32

typedef struct {
    CARD8 type;				/* always evBase + RRNotify */
    CARD8 subCode;			/* RRNotify_Lease */
    CARD16 sequenceNumber B16;
    Time timestamp B32;			/* time resource was changed */
    Window window B32;			/* window requesting notification */
    RRLease lease B32;
    CARD8 created;			/* created/deleted */
    CARD8 pad0;
    CARD16 pad1 B16;
    CARD32 pad2 B32;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
} xRRLeaseNotifyEvent;
#define sz_xRRLeaseNotifyEvent		32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRCrtc	crtc B32;
} xRRGetPanningReq; 
#define sz_xRRGetPanningReq		8

typedef struct {
    BYTE	type;
    CARD8	status;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Time	timestamp B32;
    CARD16	left B16;
    CARD16	top B16;
    CARD16	width B16;
    CARD16	height B16;
    CARD16	track_left B16;
    CARD16	track_top B16;
    CARD16	track_width B16;
    CARD16	track_height B16;
    INT16	border_left B16;
    INT16	border_top B16;
    INT16	border_right B16;
    INT16	border_bottom B16;
} xRRGetPanningReply;
#define sz_xRRGetPanningReply		36

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    RRCrtc	crtc B32;
    Time	timestamp B32;
    CARD16	left B16;
    CARD16	top B16;
    CARD16	width B16;
    CARD16	height B16;
    CARD16	track_left B16;
    CARD16	track_top B16;
    CARD16	track_width B16;
    CARD16	track_height B16;
    INT16	border_left B16;
    INT16	border_top B16;
    INT16	border_right B16;
    INT16	border_bottom B16;
} xRRSetPanningReq; 
#define sz_xRRSetPanningReq		36

typedef struct {
    BYTE	type;
    CARD8	status;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Time	newTimestamp B32;
    CARD32      pad1 B32;
    CARD32      pad2 B32;
    CARD32      pad3 B32;
    CARD32      pad4 B32;
    CARD32      pad5 B32;
} xRRSetPanningReply;
#define sz_xRRSetPanningReply	32

typedef struct {
    Atom	name B32;
    BOOL	primary;
    BOOL	automatic;
    CARD16	noutput B16;
    INT16	x B16;
    INT16	y B16;
    CARD16	width B16;
    CARD16	height B16;
    CARD32	widthInMillimeters B32;
    CARD32	heightInMillimeters B32;
} xRRMonitorInfo;
#define sz_xRRMonitorInfo	24

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    Window	window B32;
    BOOL	get_active;
    CARD8	pad;
    CARD16	pad2;
} xRRGetMonitorsReq;
#define sz_xRRGetMonitorsReq	12

typedef struct {
    BYTE	type;
    CARD8	status;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    Time	timestamp B32;
    CARD32	nmonitors B32;
    CARD32	noutputs B32;
    CARD32      pad1 B32;
    CARD32      pad2 B32;
    CARD32      pad3 B32;
} xRRGetMonitorsReply;
#define sz_xRRGetMonitorsReply	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    Window	window B32;
    xRRMonitorInfo	monitor;
} xRRSetMonitorReq;
#define sz_xRRSetMonitorReq	32

typedef struct {
    CARD8	reqType;
    CARD8	randrReqType;
    CARD16	length B16;
    Window	window B32;
    Atom	name B32;
} xRRDeleteMonitorReq;
#define sz_xRRDeleteMonitorReq	12

#undef RRModeFlags
#undef RRCrtc
#undef RRMode
#undef RROutput
#undef RRMode
#undef RRCrtc
#undef RRProvider
#undef Drawable
#undef Window
#undef Font
#undef Pixmap
#undef Cursor
#undef Colormap
#undef GContext
#undef Atom
#undef Time
#undef KeyCode
#undef KeySym
#undef Rotation
#undef SizeID
#undef SubpixelOrder

#endif /* _XRANDRP_H_ */
