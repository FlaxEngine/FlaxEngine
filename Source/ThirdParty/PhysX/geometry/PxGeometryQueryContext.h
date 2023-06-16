// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2023 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef PX_GEOMETRY_QUERY_CONTEXT_H
#define PX_GEOMETRY_QUERY_CONTEXT_H

#include "common/PxPhysXCommonConfig.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	/**
	\brief A per-thread context passed to low-level query functions.

	This is a user-defined optional parameter that gets passed down to low-level query functions (raycast / overlap / sweep).

	This is not used directly in PhysX, although the context in this case is the PxHitCallback used in the query. This allows
	user-defined query functions, such as the ones from PxCustomGeometry, to get some additional data about the query. In this
	case this is a 'per-query' context rather than 'per-thread', but the initial goal of this parameter is to give custom
	query callbacks access to per-thread data structures (e.g. caches) that could be needed to implement the callbacks.

	In any case this is mostly for user-controlled query systems.
	*/
	struct PxQueryThreadContext
	{
	};

	/**
	\brief A per-thread context passed to low-level raycast functions.
	*/
	typedef PxQueryThreadContext PxRaycastThreadContext;

	/**
	\brief A per-thread context passed to low-level overlap functions.
	*/
	typedef PxQueryThreadContext PxOverlapThreadContext;

	/**
	\brief A per-thread context passed to low-level sweep functions.
	*/
	typedef PxQueryThreadContext PxSweepThreadContext;

#if !PX_DOXYGEN
}
#endif

#endif
