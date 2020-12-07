/*
   Copyright (c) 2002  XFree86 Inc
*/

#ifndef _XRESPROTO_H
#define _XRESPROTO_H

#define XRES_MAJOR_VERSION 1
#define XRES_MINOR_VERSION 2

#define XRES_NAME "X-Resource"

/* v1.0 */
#define X_XResQueryVersion            0
#define X_XResQueryClients            1
#define X_XResQueryClientResources    2
#define X_XResQueryClientPixmapBytes  3

/* Version 1.1 has been accidentally released from the version           */
/* control and while it doesn't have differences to version 1.0, the     */
/* next version is labeled 1.2 in order to remove the risk of confusion. */

/* v1.2 */
#define X_XResQueryClientIds          4
#define X_XResQueryResourceBytes      5

typedef struct {
   CARD32 resource_base;
   CARD32 resource_mask;
} xXResClient;
#define sz_xXResClient 8

typedef struct {
   CARD32 resource_type;
   CARD32 count;
} xXResType;
#define sz_xXResType 8

/* XResQueryVersion */

typedef struct _XResQueryVersion {
   CARD8   reqType;
   CARD8   XResReqType; 
   CARD16  length B16;
   CARD8   client_major;
   CARD8   client_minor;
   CARD16  unused B16;           
} xXResQueryVersionReq;
#define sz_xXResQueryVersionReq 8

typedef struct {
   CARD8   type;
   CARD8   pad1;
   CARD16  sequenceNumber B16; 
   CARD32  length B32;
   CARD16  server_major B16;      
   CARD16  server_minor B16;      
   CARD32  pad2 B32;
   CARD32  pad3 B32;
   CARD32  pad4 B32;
   CARD32  pad5 B32;
   CARD32  pad6 B32; 
} xXResQueryVersionReply;
#define sz_xXResQueryVersionReply  32

/* XResQueryClients */

typedef struct _XResQueryClients {
   CARD8   reqType;
   CARD8   XResReqType;       
   CARD16  length B16;
} xXResQueryClientsReq;
#define sz_xXResQueryClientsReq 4

typedef struct {
   CARD8   type;
   CARD8   pad1;     
   CARD16  sequenceNumber B16;  
   CARD32  length B32;
   CARD32  num_clients B32;
   CARD32  pad2 B32;
   CARD32  pad3 B32;
   CARD32  pad4 B32;
   CARD32  pad5 B32;
   CARD32  pad6 B32;        
} xXResQueryClientsReply;
#define sz_xXResQueryClientsReply  32

/* XResQueryClientResources */

typedef struct _XResQueryClientResources {
   CARD8   reqType;
   CARD8   XResReqType;
   CARD16  length B16;
   CARD32  xid B32;
} xXResQueryClientResourcesReq;
#define sz_xXResQueryClientResourcesReq 8

typedef struct {
   CARD8   type;
   CARD8   pad1;     
   CARD16  sequenceNumber B16;  
   CARD32  length B32;
   CARD32  num_types B32;
   CARD32  pad2 B32;
   CARD32  pad3 B32;
   CARD32  pad4 B32;
   CARD32  pad5 B32;
   CARD32  pad6 B32;
} xXResQueryClientResourcesReply;
#define sz_xXResQueryClientResourcesReply  32

/* XResQueryClientPixmapBytes */

typedef struct _XResQueryClientPixmapBytes {
   CARD8   reqType;
   CARD8   XResReqType;
   CARD16  length B16;
   CARD32  xid B32;
} xXResQueryClientPixmapBytesReq;
#define sz_xXResQueryClientPixmapBytesReq 8

typedef struct {
   CARD8   type;
   CARD8   pad1;
   CARD16  sequenceNumber B16;
   CARD32  length B32;
   CARD32  bytes B32;
   CARD32  bytes_overflow B32;
   CARD32  pad2 B32;
   CARD32  pad3 B32;
   CARD32  pad4 B32;
   CARD32  pad5 B32;
} xXResQueryClientPixmapBytesReply;
#define sz_xXResQueryClientPixmapBytesReply  32

/* v1.2 XResQueryClientIds */

#define X_XResClientXIDMask      0x01
#define X_XResLocalClientPIDMask 0x02

typedef struct _XResClientIdSpec {
   CARD32  client B32;
   CARD32  mask B32;
} xXResClientIdSpec;
#define sz_xXResClientIdSpec 8

typedef struct _XResClientIdValue {
   xXResClientIdSpec spec;
   CARD32  length B32;
   // followed by length CARD32s
} xXResClientIdValue;
#define sz_xResClientIdValue (sz_xXResClientIdSpec + 4)

typedef struct _XResQueryClientIds {
   CARD8   reqType;
   CARD8   XResReqType;
   CARD16  length B16;
   CARD32  numSpecs B32;
   // followed by numSpecs times XResClientIdSpec
} xXResQueryClientIdsReq;
#define sz_xXResQueryClientIdsReq 8

typedef struct {
   CARD8   type;
   CARD8   pad1;
   CARD16  sequenceNumber B16;
   CARD32  length B32;
   CARD32  numIds B32;
   CARD32  pad2 B32;
   CARD32  pad3 B32;
   CARD32  pad4 B32;
   CARD32  pad5 B32;
   CARD32  pad6 B32;
   // followed by numIds times XResClientIdValue
} xXResQueryClientIdsReply;
#define sz_xXResQueryClientIdsReply  32

/* v1.2 XResQueryResourceBytes */

typedef struct _XResResourceIdSpec {
   CARD32  resource;
   CARD32  type;
} xXResResourceIdSpec;
#define sz_xXResResourceIdSpec 8

typedef struct _XResQueryResourceBytes {
   CARD8   reqType;
   CARD8   XResReqType;
   CARD16  length B16;
   CARD32  client B32;
   CARD32  numSpecs B32;
   // followed by numSpecs times XResResourceIdSpec
} xXResQueryResourceBytesReq;
#define sz_xXResQueryResourceBytesReq 12

typedef struct _XResResourceSizeSpec {
   xXResResourceIdSpec spec;
   CARD32  bytes B32;
   CARD32  refCount B32;
   CARD32  useCount B32;
} xXResResourceSizeSpec;
#define sz_xXResResourceSizeSpec (sz_xXResResourceIdSpec + 12)

typedef struct _XResResourceSizeValue {
   xXResResourceSizeSpec size;
   CARD32  numCrossReferences B32;
   // followed by numCrossReferences times XResResourceSizeSpec
} xXResResourceSizeValue;
#define sz_xXResResourceSizeValue (sz_xXResResourceSizeSpec + 4)

typedef struct {
   CARD8   type;
   CARD8   pad1;
   CARD16  sequenceNumber B16;
   CARD32  length B32;
   CARD32  numSizes B32;
   CARD32  pad2 B32;
   CARD32  pad3 B32;
   CARD32  pad4 B32;
   CARD32  pad5 B32;
   CARD32  pad6 B32;
   // followed by numSizes times XResResourceSizeValue
} xXResQueryResourceBytesReply;
#define sz_xXResQueryResourceBytesReply  32

#endif /* _XRESPROTO_H */
