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

#ifndef PX_CONTACTBUFFER_H
#define PX_CONTACTBUFFER_H

#include "PxPhysXConfig.h"
#include "geomutils/PxContactPoint.h"
#include "PxContact.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

	class PxContactBuffer
	{
	public:

		static const PxU32 MAX_CONTACTS = 64;

		PxContactPoint	contacts[MAX_CONTACTS];
		PxU32			count;
		PxU32			pad;

		PX_FORCE_INLINE void reset()
		{
			count = 0;
		}

		PX_FORCE_INLINE bool contact(const PxVec3& worldPoint, 
					 const PxVec3& worldNormalIn, 
					 PxReal separation, 
					 PxU32 faceIndex1 = PXC_CONTACT_NO_FACE_INDEX
					 )
		{
			PX_ASSERT(PxAbs(worldNormalIn.magnitude()-1)<1e-3f);

			if(count>=MAX_CONTACTS)
				return false;

			PxContactPoint& p	= contacts[count++];
			p.normal			= worldNormalIn;
			p.point				= worldPoint;
			p.separation		= separation;
			p.internalFaceIndex1= faceIndex1;
			return true;
		}

		PX_FORCE_INLINE bool contact(const PxContactPoint& pt)
		{
			if(count>=MAX_CONTACTS)
				return false;
			contacts[count++] = pt;
			return true;
		}

		PX_FORCE_INLINE PxContactPoint* contact()
		{
			if(count>=MAX_CONTACTS)
				return NULL;
			return &contacts[count++];
		}
	};

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif
