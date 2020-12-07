#ifndef _XVMCPROTO_H_
#define _XVMCPROTO_H_

#define xvmc_QueryVersion		0
#define xvmc_ListSurfaceTypes		1
#define xvmc_CreateContext		2
#define xvmc_DestroyContext		3
#define xvmc_CreateSurface		4
#define xvmc_DestroySurface		5
#define xvmc_CreateSubpicture		6
#define xvmc_DestroySubpicture		7
#define xvmc_ListSubpictureTypes	8
#define xvmc_GetDRInfo                  9
#define xvmc_LastRequest		xvmc_GetDRInfo

#define xvmcNumRequest			(xvmc_LastRequest + 1)


typedef struct {
  CARD32 surface_type_id B32;
  CARD16 chroma_format B16;
  CARD16 pad0 B16;
  CARD16 max_width B16;
  CARD16 max_height B16;
  CARD16 subpicture_max_width B16;
  CARD16 subpicture_max_height B16;
  CARD32 mc_type B32;
  CARD32 flags B32;
} xvmcSurfaceInfo;
#define sz_xvmcSurfaceInfo 24;

typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
} xvmcQueryVersionReq;
#define sz_xvmcQueryVersionReq 4;

typedef struct {
  BYTE type;  /* X_Reply */
  BYTE padb1;
  CARD16 sequenceNumber B16;
  CARD32 length B32;
  CARD32 major B32;
  CARD32 minor B32;
  CARD32 padl4 B32;
  CARD32 padl5 B32;
  CARD32 padl6 B32;
  CARD32 padl7 B32;
} xvmcQueryVersionReply;
#define sz_xvmcQueryVersionReply 32

typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
  CARD32 port B32;
} xvmcListSurfaceTypesReq;
#define sz_xvmcListSurfaceTypesReq 8;

typedef struct {
  BYTE type;  /* X_Reply */
  BYTE padb1;
  CARD16 sequenceNumber B16;
  CARD32 length B32;
  CARD32 num   B32;
  CARD32 padl3 B32;
  CARD32 padl4 B32;
  CARD32 padl5 B32;
  CARD32 padl6 B32;
  CARD32 padl7 B32;
} xvmcListSurfaceTypesReply;
#define sz_xvmcListSurfaceTypesReply 32

typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
  CARD32 context_id B32;
  CARD32 port B32;
  CARD32 surface_type_id B32;
  CARD16 width B16;
  CARD16 height B16;
  CARD32 flags B32;
} xvmcCreateContextReq;
#define sz_xvmcCreateContextReq 24;

typedef struct {
  BYTE type;  /* X_Reply */
  BYTE padb1;
  CARD16 sequenceNumber B16;
  CARD32 length B32;
  CARD16 width_actual B16;
  CARD16 height_actual B16;
  CARD32 flags_return B32;
  CARD32 padl4 B32;
  CARD32 padl5 B32;
  CARD32 padl6 B32;
  CARD32 padl7 B32;
} xvmcCreateContextReply;
#define sz_xvmcCreateContextReply 32

typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
  CARD32 context_id B32;
} xvmcDestroyContextReq;
#define sz_xvmcDestroyContextReq 8;

typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
  CARD32 surface_id B32;
  CARD32 context_id B32;
} xvmcCreateSurfaceReq;
#define sz_xvmcCreateSurfaceReq 12;

typedef struct {
  BYTE type;  /* X_Reply */
  BYTE padb1;
  CARD16 sequenceNumber B16;
  CARD32 length B32;
  CARD32 padl2 B32;
  CARD32 padl3 B32;
  CARD32 padl4 B32;
  CARD32 padl5 B32;
  CARD32 padl6 B32;
  CARD32 padl7 B32;
} xvmcCreateSurfaceReply;
#define sz_xvmcCreateSurfaceReply 32

typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
  CARD32 surface_id B32;
} xvmcDestroySurfaceReq;
#define sz_xvmcDestroySurfaceReq 8;


typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
  CARD32 subpicture_id B32;
  CARD32 context_id B32;
  CARD32 xvimage_id B32;
  CARD16 width B16;
  CARD16 height B16;
} xvmcCreateSubpictureReq;
#define sz_xvmcCreateSubpictureReq 20;

typedef struct {
  BYTE type;  /* X_Reply */
  BYTE padb1;
  CARD16 sequenceNumber B16;
  CARD32 length B32;
  CARD16 width_actual B16;
  CARD16 height_actual B16;
  CARD16 num_palette_entries B16;
  CARD16 entry_bytes B16;
  CARD8  component_order[4];
  CARD32 padl5 B32;
  CARD32 padl6 B32;
  CARD32 padl7 B32;
} xvmcCreateSubpictureReply;
#define sz_xvmcCreateSubpictureReply 32

typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
  CARD32 subpicture_id B32;
} xvmcDestroySubpictureReq;
#define sz_xvmcDestroySubpictureReq 8;

typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
  CARD32 port B32;
  CARD32 surface_type_id B32;
} xvmcListSubpictureTypesReq;
#define sz_xvmcListSubpictureTypesReq 12;

typedef struct {
  BYTE type;  /* X_Reply */
  BYTE padb1;
  CARD16 sequenceNumber B16;
  CARD32 length B32;
  CARD32 num B32;
  CARD32 padl2 B32;
  CARD32 padl3 B32;
  CARD32 padl4 B32;
  CARD32 padl5 B32;
  CARD32 padl6 B32;
} xvmcListSubpictureTypesReply;
#define sz_xvmcListSubpictureTypesReply 32

typedef struct {
  CARD8 reqType;
  CARD8 xvmcReqType;
  CARD16 length B16;
  CARD32 port B32;
  CARD32 shmKey B32;
  CARD32 magic B32;
} xvmcGetDRInfoReq;
#define sz_xvmcGetDRInfoReq 16;

typedef struct {
  BYTE type;  /* X_Reply */
  BYTE padb1;
  CARD16 sequenceNumber B16;
  CARD32 length B32;
  CARD32 major B32;
  CARD32 minor B32;
  CARD32 patchLevel B32;
  CARD32 nameLen B32;
  CARD32 busIDLen B32;
  CARD32 isLocal B32;
} xvmcGetDRInfoReply;
#define sz_xvmcGetDRInfoReply 32

#endif
