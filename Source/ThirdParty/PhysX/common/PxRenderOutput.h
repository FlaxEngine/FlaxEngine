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

#ifndef PX_RENDER_OUTPUT_H
#define PX_RENDER_OUTPUT_H

#include "foundation/PxMat44.h"
#include "foundation/PxBasicTemplates.h"
#include "PxRenderBuffer.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

#if PX_VC 
#pragma warning(push)
#pragma warning( disable : 4251 ) // class needs to have dll-interface to be used by clients of class
#endif

	/**
	Output stream to fill RenderBuffer
	*/
	class PX_PHYSX_COMMON_API PxRenderOutput
	{
	public:

		enum Primitive 
		{
			POINTS,
			LINES,
			LINESTRIP,
			TRIANGLES,
			TRIANGLESTRIP
		};

		PxRenderOutput(PxRenderBuffer& buffer)
		:	mPrim(POINTS), 
			mColor(0), 
			mVertex0(0.0f), 
			mVertex1(0.0f),
			mVertexCount(0), 
			mTransform(PxIdentity),
			mBuffer(buffer)
		{
		}

		PX_INLINE PxRenderOutput& operator<<(Primitive prim);

		PX_INLINE PxRenderOutput& operator<<(PxU32 color) ;

		PX_INLINE PxRenderOutput& operator<<(const PxMat44& transform);

		PX_INLINE PxRenderOutput& operator<<(const PxTransform& t);

		PX_INLINE PxRenderOutput& operator<<(const PxVec3& vertex);	//AM: Don't use this! Slow! Deprecated!

		PX_INLINE PxDebugLine* reserveSegments(PxU32 nbSegments);

		PX_INLINE PxDebugPoint* reservePoints(PxU32 nbSegments);

		PX_INLINE void outputSegment(const PxVec3& v0, const PxVec3& v1);

		PX_INLINE PxRenderOutput& outputCapsule(PxF32 radius, PxF32 halfHeight, const PxMat44& absPose);

	private:
	
		PxRenderOutput& operator=(const PxRenderOutput&);

		Primitive		mPrim;
		PxU32			mColor;
		PxVec3			mVertex0, mVertex1;
		PxU32			mVertexCount;
		PxMat44			mTransform;
		PxRenderBuffer& mBuffer;
	};

	struct PxDebugBox
	{
		explicit PxDebugBox(const PxVec3& extents, bool wireframe_ = true)
			: minimum(-extents), maximum(extents), wireframe(wireframe_) {}

		explicit PxDebugBox(const PxVec3& pos, const PxVec3& extents, bool wireframe_ = true)
			: minimum(pos - extents), maximum(pos + extents), wireframe(wireframe_) {}

		explicit PxDebugBox(const PxBounds3& bounds, bool wireframe_ = true)
			: minimum(bounds.minimum), maximum(bounds.maximum), wireframe(wireframe_) {}

		PxVec3 minimum, maximum;
		bool wireframe;
	};
	PX_FORCE_INLINE PxRenderOutput& operator<<(PxRenderOutput& out, const PxDebugBox& box)
	{
		if (box.wireframe)
		{
			out << PxRenderOutput::LINESTRIP;
			out << PxVec3(box.minimum.x, box.minimum.y, box.minimum.z);
			out << PxVec3(box.maximum.x, box.minimum.y, box.minimum.z);
			out << PxVec3(box.maximum.x, box.maximum.y, box.minimum.z);
			out << PxVec3(box.minimum.x, box.maximum.y, box.minimum.z);
			out << PxVec3(box.minimum.x, box.minimum.y, box.minimum.z);
			out << PxVec3(box.minimum.x, box.minimum.y, box.maximum.z);
			out << PxVec3(box.maximum.x, box.minimum.y, box.maximum.z);
			out << PxVec3(box.maximum.x, box.maximum.y, box.maximum.z);
			out << PxVec3(box.minimum.x, box.maximum.y, box.maximum.z);
			out << PxVec3(box.minimum.x, box.minimum.y, box.maximum.z);
			out << PxRenderOutput::LINES;
			out << PxVec3(box.maximum.x, box.minimum.y, box.minimum.z);
			out << PxVec3(box.maximum.x, box.minimum.y, box.maximum.z);
			out << PxVec3(box.maximum.x, box.maximum.y, box.minimum.z);
			out << PxVec3(box.maximum.x, box.maximum.y, box.maximum.z);
			out << PxVec3(box.minimum.x, box.maximum.y, box.minimum.z);
			out << PxVec3(box.minimum.x, box.maximum.y, box.maximum.z);
		}
		else
		{
			out << PxRenderOutput::TRIANGLESTRIP;
			out << PxVec3(box.minimum.x, box.minimum.y, box.minimum.z); // 0
			out << PxVec3(box.minimum.x, box.maximum.y, box.minimum.z); // 2
			out << PxVec3(box.maximum.x, box.minimum.y, box.minimum.z); // 1
			out << PxVec3(box.maximum.x, box.maximum.y, box.minimum.z); // 3
			out << PxVec3(box.maximum.x, box.maximum.y, box.maximum.z); // 7
			out << PxVec3(box.minimum.x, box.maximum.y, box.minimum.z); // 2
			out << PxVec3(box.minimum.x, box.maximum.y, box.maximum.z); // 6
			out << PxVec3(box.minimum.x, box.minimum.y, box.minimum.z); // 0
			out << PxVec3(box.minimum.x, box.minimum.y, box.maximum.z); // 4
			out << PxVec3(box.maximum.x, box.minimum.y, box.minimum.z); // 1
			out << PxVec3(box.maximum.x, box.minimum.y, box.maximum.z); // 5
			out << PxVec3(box.maximum.x, box.maximum.y, box.maximum.z); // 7
			out << PxVec3(box.minimum.x, box.minimum.y, box.maximum.z); // 4
			out << PxVec3(box.minimum.x, box.maximum.y, box.maximum.z); // 6
		}
		return out;
	}

	struct PxDebugArrow
	{
		PxDebugArrow(const PxVec3& pos, const PxVec3& vec)
			: base(pos), tip(pos + vec), headLength(vec.magnitude()*0.15f) {}

		PxDebugArrow(const PxVec3& pos, const PxVec3& vec, PxReal headLength_)
			: base(pos), tip(pos + vec), headLength(headLength_) {}

		PxVec3 base, tip;
		PxReal headLength;
	};
	PX_FORCE_INLINE void normalToTangents(const PxVec3& normal, PxVec3& tangent0, PxVec3& tangent1)
	{
		tangent0 = PxAbs(normal.x) < 0.70710678f ? PxVec3(0, -normal.z, normal.y) : PxVec3(-normal.y, normal.x, 0);
		tangent0.normalize();
		tangent1 = normal.cross(tangent0);
	}
	PX_FORCE_INLINE PxRenderOutput& operator<<(PxRenderOutput& out, const PxDebugArrow& arrow)
	{
		PxVec3 t0 = arrow.tip - arrow.base, t1, t2;

		t0.normalize();
		normalToTangents(t0, t1, t2);

		const PxReal tipAngle = 0.25f;
		t1 *= arrow.headLength * tipAngle;
		t2 *= arrow.headLength * tipAngle * PxSqrt(3.0f);
		PxVec3 headBase = arrow.tip - t0 * arrow.headLength;

		out << PxRenderOutput::LINES;
		out << arrow.base << arrow.tip;

		out << PxRenderOutput::TRIANGLESTRIP;
		out << arrow.tip;
		out << headBase + t1 + t1;
		out << headBase - t1 - t2;
		out << headBase - t1 + t2;
		out << arrow.tip;
		out << headBase + t1 + t1;
		return out;
	}

	struct PxDebugBasis
	{
		PxDebugBasis(const PxVec3& ext, PxU32 cX = PxU32(PxDebugColor::eARGB_RED),
			PxU32 cY = PxU32(PxDebugColor::eARGB_GREEN), PxU32 cZ = PxU32(PxDebugColor::eARGB_BLUE))
			: extends(ext), colorX(cX), colorY(cY), colorZ(cZ) {}
		PxVec3 extends;
		PxU32 colorX, colorY, colorZ;
	};
	PX_FORCE_INLINE PxRenderOutput& operator<<(PxRenderOutput& out, const PxDebugBasis& basis)
	{
		const PxReal headLength = basis.extends.magnitude() * 0.15f;
		out << basis.colorX << PxDebugArrow(PxVec3(0.0f), PxVec3(basis.extends.x, 0, 0), headLength);
		out << basis.colorY << PxDebugArrow(PxVec3(0.0f), PxVec3(0, basis.extends.y, 0), headLength);
		out << basis.colorZ << PxDebugArrow(PxVec3(0.0f), PxVec3(0, 0, basis.extends.z), headLength);
		return out;
	}

	struct PxDebugCircle
	{
		PxDebugCircle(PxU32 s, PxReal r)
			: nSegments(s), radius(r) {}
		PxU32 nSegments;
		PxReal radius;
	};
	PX_FORCE_INLINE PxRenderOutput& operator<<(PxRenderOutput& out, const PxDebugCircle& circle)
	{
		const PxF32 step = PxTwoPi / circle.nSegments;
		PxF32 angle = 0;
		out << PxRenderOutput::LINESTRIP;
		for (PxU32 i = 0; i < circle.nSegments; i++, angle += step)
			out << PxVec3(circle.radius * PxSin(angle), circle.radius * PxCos(angle), 0);
		out << PxVec3(0, circle.radius, 0);
		return out;
	}

	struct PxDebugArc
	{
		PxDebugArc(PxU32 s, PxReal r, PxReal minAng, PxReal maxAng)
			: nSegments(s), radius(r), minAngle(minAng), maxAngle(maxAng) {}
		PxU32 nSegments;
		PxReal radius;
		PxReal minAngle, maxAngle;
	};
	PX_FORCE_INLINE PxRenderOutput& operator<<(PxRenderOutput& out, const PxDebugArc& arc)
	{
		const PxF32 step = (arc.maxAngle - arc.minAngle) / arc.nSegments;
		PxF32 angle = arc.minAngle;
		out << PxRenderOutput::LINESTRIP;
		for (PxU32 i = 0; i < arc.nSegments; i++, angle += step)
			out << PxVec3(arc.radius * PxSin(angle), arc.radius * PxCos(angle), 0);
		out << PxVec3(arc.radius * PxSin(arc.maxAngle), arc.radius * PxCos(arc.maxAngle), 0);
		return out;
	}

	PX_INLINE PxRenderOutput& PxRenderOutput::operator<<(Primitive prim)
	{
		mPrim = prim;
		mVertexCount = 0;
		return *this;
	}

	PX_INLINE PxRenderOutput& PxRenderOutput::operator<<(PxU32 color)
	{
		mColor = color;
		return *this;
	}

	PX_INLINE PxRenderOutput& PxRenderOutput::operator<<(const PxMat44& transform)
	{
		mTransform = transform;
		return *this;
	}

	PX_INLINE PxRenderOutput& PxRenderOutput::operator<<(const PxTransform& t)
	{
		mTransform = PxMat44(t);
		return *this;
	}

	PX_INLINE PxRenderOutput& PxRenderOutput::operator<<(const PxVec3& vertexIn)
	{
		// apply transformation
		const PxVec3 vertex = mTransform.transform(vertexIn);
		++mVertexCount;

		// add primitive to render buffer
		switch (mPrim)
		{
		case POINTS:
			mBuffer.addPoint(PxDebugPoint(vertex, mColor)); break;
		case LINES:
			if (mVertexCount == 2)
			{
				mBuffer.addLine(PxDebugLine(mVertex0, vertex, mColor));
				mVertexCount = 0;
			}
			break;
		case LINESTRIP:
			if (mVertexCount >= 2)
				mBuffer.addLine(PxDebugLine(mVertex0, vertex, mColor));
			break;
		case TRIANGLES:
			if (mVertexCount == 3)
			{
				mBuffer.addTriangle(PxDebugTriangle(mVertex1, mVertex0, vertex, mColor));
				mVertexCount = 0;
			}
			break;
		case TRIANGLESTRIP:
			if (mVertexCount >= 3)
				mBuffer.addTriangle(PxDebugTriangle(
				(mVertexCount & 0x1) ? mVertex0 : mVertex1,
					(mVertexCount & 0x1) ? mVertex1 : mVertex0, vertex, mColor));
			break;
		}

		// cache the last 2 vertices (for strips)
		if (1 < mVertexCount)
		{
			mVertex1 = mVertex0;
			mVertex0 = vertex;
		}
		else
		{
			mVertex0 = vertex;
		}
		return *this;
	}

	PX_INLINE PxDebugLine* PxRenderOutput::reserveSegments(PxU32 nbSegments)
	{
		return mBuffer.reserveLines(nbSegments);
	}

	PX_INLINE PxDebugPoint* PxRenderOutput::reservePoints(PxU32 nbPoints)
	{
		return mBuffer.reservePoints(nbPoints);
	}

	// PT: using the operators is just too slow.
	PX_INLINE void PxRenderOutput::outputSegment(const PxVec3& v0, const PxVec3& v1)
	{
		PxDebugLine* segment = mBuffer.reserveLines(1);
		segment->pos0 = v0;
		segment->pos1 = v1;
		segment->color0 = segment->color1 = mColor;
	}

	PX_INLINE PxRenderOutput& PxRenderOutput::outputCapsule(PxF32 radius, PxF32 halfHeight, const PxMat44& absPose)
	{
		PxRenderOutput& out = *this;

		const PxVec3 vleft2(-halfHeight, 0.0f, 0.0f);
		PxMat44 left2 = absPose;
		left2.column3 += PxVec4(left2.rotate(vleft2), 0.0f);
		out << left2 << PxDebugArc(100, radius, PxPi, PxTwoPi);

		PxMat44 rotPose = left2;
		PxSwap(rotPose.column1, rotPose.column2);
		rotPose.column1 = -rotPose.column1;
		out << rotPose << PxDebugArc(100, radius, PxPi, PxTwoPi);

		PxSwap(rotPose.column0, rotPose.column2);
		rotPose.column0 = -rotPose.column0;
		out << rotPose << PxDebugCircle(100, radius);

		const PxVec3 vright2(halfHeight, 0.0f, 0.0f);
		PxMat44 right2 = absPose;
		right2.column3 += PxVec4(right2.rotate(vright2), 0.0f);
		out << right2 << PxDebugArc(100, radius, 0.0f, PxPi);

		rotPose = right2;
		PxSwap(rotPose.column1, rotPose.column2);
		rotPose.column1 = -rotPose.column1;
		out << rotPose << PxDebugArc(100, radius, 0.0f, PxPi);

		PxSwap(rotPose.column0, rotPose.column2);
		rotPose.column0 = -rotPose.column0;
		out << rotPose << PxDebugCircle(100, radius);

		out << absPose;
		out.outputSegment(absPose.transform(PxVec3(-halfHeight, radius, 0)),
			absPose.transform(PxVec3(halfHeight, radius, 0)));
		out.outputSegment(absPose.transform(PxVec3(-halfHeight, -radius, 0)),
			absPose.transform(PxVec3(halfHeight, -radius, 0)));
		out.outputSegment(absPose.transform(PxVec3(-halfHeight, 0, radius)),
			absPose.transform(PxVec3(halfHeight, 0, radius)));
		out.outputSegment(absPose.transform(PxVec3(-halfHeight, 0, -radius)),
			absPose.transform(PxVec3(halfHeight, 0, -radius)));

		return *this;
	}

#if PX_VC 
#pragma warning(pop) 
#endif

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif
