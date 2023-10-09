// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#pragma once

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace nv
{
namespace cloth
{

/** Callback interface to manage the DirectX context/device used for compute
  *
  */
class DxContextManagerCallback
{
public:
	virtual ~DxContextManagerCallback() {}
	/**
	* \brief Acquire the D3D context for the current thread
	*
	* Acquisitions are allowed to be recursive within a single thread.
	* You can acquire the context multiple times so long as you release
	* it the same count.
	*/
	virtual void acquireContext() = 0;

	/**
	* \brief Release the D3D context from the current thread
	*/
	virtual void releaseContext() = 0;

	/**
	* \brief Return the D3D device to use for compute work
	*/
	virtual ID3D11Device* getDevice() const = 0;

	/**
	* \brief Return the D3D context to use for compute work
	*/
	virtual ID3D11DeviceContext* getContext() const = 0;

	/**
	* \brief Return if exposed buffers (only cloth particles at the moment)
	* are created with D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX.
	*
	* The user is responsible to query and acquire the mutex of all
	* corresponding buffers.
	* todo: We should acquire the mutex locally if we continue to
	* allow resource sharing across devices.
	*/
	virtual bool synchronizeResources() const = 0;
};

}
}