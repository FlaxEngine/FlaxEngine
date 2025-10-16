/*********************************************************************************************************\
|*                                                                                                        *|
|* SPDX-FileCopyrightText: Copyright (c) 2019-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.  *|
|* SPDX-License-Identifier: MIT                                                                           *|
|*                                                                                                        *|
|* Permission is hereby granted, free of charge, to any person obtaining a                                *|
|* copy of this software and associated documentation files (the "Software"),                             *|
|* to deal in the Software without restriction, including without limitation                              *|
|* the rights to use, copy, modify, merge, publish, distribute, sublicense,                               *|
|* and/or sell copies of the Software, and to permit persons to whom the                                  *|
|* Software is furnished to do so, subject to the following conditions:                                   *|
|*                                                                                                        *|
|* The above copyright notice and this permission notice shall be included in                             *|
|* all copies or substantial portions of the Software.                                                    *|
|*                                                                                                        *|
|* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                             *|
|* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                               *|
|* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL                               *|
|* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                             *|
|* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING                                *|
|* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER                                    *|
|* DEALINGS IN THE SOFTWARE.                                                                              *|
|*                                                                                                        *|
|*                                                                                                        *|
\*********************************************************************************************************/

////////////////////////// NVIDIA SHADER EXTENSIONS /////////////////

// this file is to be #included in the app HLSL shader code to make
// use of nvidia shader extensions


#include "nvHLSLExtnsInternal.h"

//----------------------------------------------------------------------------//
//------------------------- Warp Shuffle Functions ---------------------------//
//----------------------------------------------------------------------------//

// all functions have variants with width parameter which permits sub-division 
// of the warp into segments - for example to exchange data between 4 groups of 
// 8 lanes in a SIMD manner. If width is less than warpSize then each subsection 
// of the warp behaves as a separate entity with a starting logical lane ID of 0. 
// A thread may only exchange data with others in its own subsection. Width must 
// have a value which is a power of 2 so that the warp can be subdivided equally; 
// results are undefined if width is not a power of 2, or is a number greater 
// than warpSize.

//
// simple variant of SHFL instruction
// returns val from the specified lane
// optional width parameter must be a power of two and width <= 32
// 
int NvShfl(int val, uint srcLane, int width = NV_WARP_SIZE)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  val;                             // variable to be shuffled
    g_NvidiaExt[index].src0u.y  =  srcLane;                         // source lane
    g_NvidiaExt[index].src0u.z  =  __NvGetShflMaskFromWidth(width);
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_SHFL;
    
    // result is returned as the return value of IncrementCounter on fake UAV slot
    return g_NvidiaExt.IncrementCounter();
}

int2 NvShfl(int2 val, uint srcLane, int width = NV_WARP_SIZE)
{
    int x = NvShfl(val.x, srcLane, width);
    int y = NvShfl(val.y, srcLane, width);
    return int2(x, y);
}

int4 NvShfl(int4 val, uint srcLane, int width = NV_WARP_SIZE)
{
    int x = NvShfl(val.x, srcLane, width);
    int y = NvShfl(val.y, srcLane, width);
    int z = NvShfl(val.z, srcLane, width);
    int w = NvShfl(val.w, srcLane, width);
    return int4(x, y, z, w);
}

//
// Copy from a lane with lower ID relative to caller
//
int NvShflUp(int val, uint delta, int width = NV_WARP_SIZE)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  val;                        // variable to be shuffled
    g_NvidiaExt[index].src0u.y  =  delta;                      // relative lane offset
    g_NvidiaExt[index].src0u.z  =  (NV_WARP_SIZE - width) << 8;   // minIndex = maxIndex for shfl_up (src2[4:0] is expected to be 0)
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_SHFL_UP;
    return g_NvidiaExt.IncrementCounter();
}

//
// Copy from a lane with higher ID relative to caller
//
int NvShflDown(int val, uint delta, int width = NV_WARP_SIZE)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  val;           // variable to be shuffled
    g_NvidiaExt[index].src0u.y  =  delta;         // relative lane offset
    g_NvidiaExt[index].src0u.z  =  __NvGetShflMaskFromWidth(width);
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_SHFL_DOWN;
    return g_NvidiaExt.IncrementCounter();
}

//
// Copy from a lane based on bitwise XOR of own lane ID
//
int NvShflXor(int val, uint laneMask, int width = NV_WARP_SIZE)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  val;           // variable to be shuffled
    g_NvidiaExt[index].src0u.y  =  laneMask;      // laneMask to be XOR'ed with current laneId to get the source lane id
    g_NvidiaExt[index].src0u.z  =  __NvGetShflMaskFromWidth(width); 
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_SHFL_XOR;
    return g_NvidiaExt.IncrementCounter();
}


//----------------------------------------------------------------------------//
//----------------------------- Warp Vote Functions---------------------------//
//----------------------------------------------------------------------------//

// returns 0xFFFFFFFF if the predicate is true for any thread in the warp, returns 0 otherwise
uint NvAny(int predicate)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  predicate;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_VOTE_ANY;
    return g_NvidiaExt.IncrementCounter();
}

// returns 0xFFFFFFFF if the predicate is true for ALL threads in the warp, returns 0 otherwise
uint NvAll(int predicate)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  predicate;
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_VOTE_ALL;
    return g_NvidiaExt.IncrementCounter();
}

// returns a mask of all threads in the warp with bits set for threads that have predicate true
uint NvBallot(int predicate)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  predicate;
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_VOTE_BALLOT;
    return g_NvidiaExt.IncrementCounter();
}


//----------------------------------------------------------------------------//
//----------------------------- Utility Functions ----------------------------//
//----------------------------------------------------------------------------//

// returns the lane index of the current thread (thread index in warp)
int NvGetLaneId()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_GET_LANE_ID;
    return g_NvidiaExt.IncrementCounter();
}

// returns value of special register - specify subopcode from any of NV_SPECIALOP_* specified in nvShaderExtnEnums.h - other opcodes undefined behavior
uint NvGetSpecial(uint subOpCode)
{
    return __NvGetSpecial(subOpCode);
}

//----------------------------------------------------------------------------//
//----------------------------- FP16 Atmoic Functions-------------------------//
//----------------------------------------------------------------------------//

// The functions below performs atomic operations on two consecutive fp16 
// values in the given raw UAV. 
// The uint paramater 'fp16x2Val' is treated as two fp16 values byteAddress must be multiple of 4
// The returned value are the two fp16 values packed into a single uint

uint NvInterlockedAddFp16x2(RWByteAddressBuffer uav, uint byteAddress, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWByteAddressBuffer uav, uint byteAddress, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWByteAddressBuffer uav, uint byteAddress, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, fp16x2Val, NV_EXTN_ATOM_MAX);
}


// versions of the above functions taking two fp32 values (internally converted to fp16 values)
uint NvInterlockedAddFp16x2(RWByteAddressBuffer uav, uint byteAddress, float2 val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWByteAddressBuffer uav, uint byteAddress, float2 val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWByteAddressBuffer uav, uint byteAddress, float2 val)
{
    return __NvAtomicOpFP16x2(uav, byteAddress, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MAX);
}


//----------------------------------------------------------------------------//

// The functions below perform atomic operation on a R16G16_FLOAT UAV at the given address
// the uint paramater 'fp16x2Val' is treated as two fp16 values
// the returned value are the two fp16 values (.x and .y components) packed into a single uint
// Warning: Behaviour of these set of functions is undefined if the UAV is not 
// of R16G16_FLOAT format (might result in app crash or TDR)

uint NvInterlockedAddFp16x2(RWTexture1D<float2> uav, uint address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture1D<float2> uav, uint address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture1D<float2> uav, uint address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

uint NvInterlockedAddFp16x2(RWTexture2D<float2> uav, uint2 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture2D<float2> uav, uint2 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture2D<float2> uav, uint2 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

uint NvInterlockedAddFp16x2(RWTexture3D<float2> uav, uint3 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture3D<float2> uav, uint3 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture3D<float2> uav, uint3 address, uint fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}


// versions taking two fp32 values (internally converted to fp16)
uint NvInterlockedAddFp16x2(RWTexture1D<float2> uav, uint address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture1D<float2> uav, uint address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture1D<float2> uav, uint address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MAX);
}

uint NvInterlockedAddFp16x2(RWTexture2D<float2> uav, uint2 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture2D<float2> uav, uint2 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture2D<float2> uav, uint2 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MAX);
}

uint NvInterlockedAddFp16x2(RWTexture3D<float2> uav, uint3 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_ADD);
}

uint NvInterlockedMinFp16x2(RWTexture3D<float2> uav, uint3 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MIN);
}

uint NvInterlockedMaxFp16x2(RWTexture3D<float2> uav, uint3 address, float2 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x2Tofp16x2(val), NV_EXTN_ATOM_MAX);
}


//----------------------------------------------------------------------------//

// The functions below perform Atomic operation on a R16G16B16A16_FLOAT UAV at the given address
// the uint2 paramater 'fp16x2Val' is treated as four fp16 values 
// i.e, fp16x2Val.x = uav.xy and fp16x2Val.y = uav.yz
// The returned value are the four fp16 values (.xyzw components) packed into uint2
// Warning: Behaviour of these set of functions is undefined if the UAV is not 
// of R16G16B16A16_FLOAT format (might result in app crash or TDR)

uint2 NvInterlockedAddFp16x4(RWTexture1D<float4> uav, uint address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture1D<float4> uav, uint address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture1D<float4> uav, uint address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedAddFp16x4(RWTexture2D<float4> uav, uint2 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture2D<float4> uav, uint2 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture2D<float4> uav, uint2 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedAddFp16x4(RWTexture3D<float4> uav, uint3 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture3D<float4> uav, uint3 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture3D<float4> uav, uint3 address, uint2 fp16x2Val)
{
    return __NvAtomicOpFP16x2(uav, address, fp16x2Val, NV_EXTN_ATOM_MAX);
}

// versions taking four fp32 values (internally converted to fp16)
uint2 NvInterlockedAddFp16x4(RWTexture1D<float4> uav, uint address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture1D<float4> uav, uint address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture1D<float4> uav, uint address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedAddFp16x4(RWTexture2D<float4> uav, uint2 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture2D<float4> uav, uint2 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture2D<float4> uav, uint2 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedAddFp16x4(RWTexture3D<float4> uav, uint3 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMinFp16x4(RWTexture3D<float4> uav, uint3 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedMaxFp16x4(RWTexture3D<float4> uav, uint3 address, float4 val)
{
    return __NvAtomicOpFP16x2(uav, address, __fp32x4Tofp16x4(val), NV_EXTN_ATOM_MAX);
}


//----------------------------------------------------------------------------//
//----------------------------- FP32 Atmoic Functions-------------------------//
//----------------------------------------------------------------------------//

// The functions below performs atomic add on the given UAV treating the value as float
// byteAddress must be multiple of 4
// The returned value is the value present in memory location before the atomic add

float NvInterlockedAddFp32(RWByteAddressBuffer uav, uint byteAddress, float val)
{
    return __NvAtomicAddFP32(uav, byteAddress, val);
}

//----------------------------------------------------------------------------//

// The functions below perform atomic add on a R32_FLOAT UAV at the given address
// the returned value is the value before performing the atomic add
// Warning: Behaviour of these set of functions is undefined if the UAV is not 
// of R32_FLOAT format (might result in app crash or TDR)

float NvInterlockedAddFp32(RWTexture1D<float> uav, uint address, float val)
{
    return __NvAtomicAddFP32(uav, address, val);
}

float NvInterlockedAddFp32(RWTexture2D<float> uav, uint2 address, float val)
{
    return __NvAtomicAddFP32(uav, address, val);
}

float NvInterlockedAddFp32(RWTexture3D<float> uav, uint3 address, float val)
{
    return __NvAtomicAddFP32(uav, address, val);
}


//----------------------------------------------------------------------------//
//--------------------------- UINT64 Atmoic Functions-------------------------//
//----------------------------------------------------------------------------//

// The functions below performs atomic operation on the given UAV treating the value as uint64
// byteAddress must be multiple of 8
// The returned value is the value present in memory location before the atomic operation
// uint2 vector type is used to represent a single uint64 value with the x component containing the low 32 bits and y component the high 32 bits.

uint2 NvInterlockedAddUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMaxUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedMinUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedAndUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_AND);
}

uint2 NvInterlockedOrUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_OR);
}

uint2 NvInterlockedXorUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_XOR);
}

uint2 NvInterlockedCompareExchangeUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 compare_value, uint2 value)
{
    return __NvAtomicCompareExchangeUINT64(uav, byteAddress, compare_value, value);
}

uint2 NvInterlockedExchangeUint64(RWByteAddressBuffer uav, uint byteAddress, uint2 value)
{
    return __NvAtomicOpUINT64(uav, byteAddress, value, NV_EXTN_ATOM_SWAP);
}

//----------------------------------------------------------------------------//

// The functions below perform atomic operation on a R32G32_UINT UAV at the given address treating the value as uint64
// the returned value is the value before performing the atomic operation
// uint2 vector type is used to represent a single uint64 value with the x component containing the low 32 bits and y component the high 32 bits.
// Warning: Behaviour of these set of functions is undefined if the UAV is not of R32G32_UINT format (might result in app crash or TDR)

uint2 NvInterlockedAddUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMaxUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedMinUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedAndUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_AND);
}

uint2 NvInterlockedOrUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_OR);
}

uint2 NvInterlockedXorUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_XOR);
}

uint2 NvInterlockedCompareExchangeUint64(RWTexture1D<uint2> uav, uint address, uint2 compare_value, uint2 value)
{
    return __NvAtomicCompareExchangeUINT64(uav, address, compare_value, value);
}

uint2 NvInterlockedExchangeUint64(RWTexture1D<uint2> uav, uint address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_SWAP);
}

uint2 NvInterlockedAddUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMaxUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedMinUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedAndUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_AND);
}

uint2 NvInterlockedOrUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_OR);
}

uint2 NvInterlockedXorUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_XOR);
}

uint2 NvInterlockedCompareExchangeUint64(RWTexture2D<uint2> uav, uint2 address, uint2 compare_value, uint2 value)
{
    return __NvAtomicCompareExchangeUINT64(uav, address, compare_value, value);
}

uint2 NvInterlockedExchangeUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_SWAP);
}

uint2 NvInterlockedAddUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_ADD);
}

uint2 NvInterlockedMaxUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MAX);
}

uint2 NvInterlockedMinUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_MIN);
}

uint2 NvInterlockedAndUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_AND);
}

uint2 NvInterlockedOrUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_OR);
}

uint2 NvInterlockedXorUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_XOR);
}

uint2 NvInterlockedCompareExchangeUint64(RWTexture3D<uint2> uav, uint3 address, uint2 compare_value, uint2 value)
{
    return __NvAtomicCompareExchangeUINT64(uav, address, compare_value, value);
}

uint2 NvInterlockedExchangeUint64(RWTexture3D<uint2> uav, uint3 address, uint2 value)
{
    return __NvAtomicOpUINT64(uav, address, value, NV_EXTN_ATOM_SWAP);
}

//----------------------------------------------------------------------------//
//--------------------------- VPRS functions ---------------------------------//
//----------------------------------------------------------------------------//

// Returns the shading rate and the number of per-pixel shading passes for current VPRS pixel
uint3 NvGetShadingRate()
{
    uint3 shadingRate = (uint3)0;
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_GET_SHADING_RATE;
    g_NvidiaExt[index].numOutputsForIncCounter = 3;
    shadingRate.x = g_NvidiaExt.IncrementCounter();
    shadingRate.y = g_NvidiaExt.IncrementCounter();
    shadingRate.z = g_NvidiaExt.IncrementCounter();
    return shadingRate;
}

float NvEvaluateAttributeAtSampleForVPRS(float attrib, uint sampleIndex, int2 pixelOffset)
{
    float value = (float)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float2 NvEvaluateAttributeAtSampleForVPRS(float2 attrib, uint sampleIndex, int2 pixelOffset)
{
    float2 value = (float2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float3 NvEvaluateAttributeAtSampleForVPRS(float3 attrib, uint sampleIndex, int2 pixelOffset)
{
    float3 value = (float3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    value.z = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float4 NvEvaluateAttributeAtSampleForVPRS(float4 attrib, uint sampleIndex, int2 pixelOffset)
{
    float4 value = (float4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    value.z = asfloat(g_NvidiaExt.IncrementCounter());
    value.w = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

int NvEvaluateAttributeAtSampleForVPRS(int attrib, uint sampleIndex, int2 pixelOffset)
{
    int value = (int)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int2 NvEvaluateAttributeAtSampleForVPRS(int2 attrib, uint sampleIndex, int2 pixelOffset)
{
    int2 value = (int2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int3 NvEvaluateAttributeAtSampleForVPRS(int3 attrib, uint sampleIndex, int2 pixelOffset)
{
    int3 value = (int3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    value.z = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int4 NvEvaluateAttributeAtSampleForVPRS(int4 attrib, uint sampleIndex, int2 pixelOffset)
{
    int4 value = (int4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    value.z = asint(g_NvidiaExt.IncrementCounter());
    value.w = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint NvEvaluateAttributeAtSampleForVPRS(uint attrib, uint sampleIndex, int2 pixelOffset)
{
    uint value = (uint)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint2 NvEvaluateAttributeAtSampleForVPRS(uint2 attrib, uint sampleIndex, int2 pixelOffset)
{
    uint2 value = (uint2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint3 NvEvaluateAttributeAtSampleForVPRS(uint3 attrib, uint sampleIndex, int2 pixelOffset)
{
    uint3 value = (uint3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    value.z = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint4 NvEvaluateAttributeAtSampleForVPRS(uint4 attrib, uint sampleIndex, int2 pixelOffset)
{
    uint4 value = (uint4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_AT_SAMPLE;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.x    = sampleIndex;
    g_NvidiaExt[ext].src2u.xy   = pixelOffset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    value.z = asuint(g_NvidiaExt.IncrementCounter());
    value.w = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}


float NvEvaluateAttributeSnappedForVPRS(float attrib, uint2 offset)
{
    float value = (float)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float2 NvEvaluateAttributeSnappedForVPRS(float2 attrib, uint2 offset)
{
    float2 value = (float2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float3 NvEvaluateAttributeSnappedForVPRS(float3 attrib, uint2 offset)
{
    float3 value = (float3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    value.z = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

float4 NvEvaluateAttributeSnappedForVPRS(float4 attrib, uint2 offset)
{
    float4 value = (float4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asfloat(g_NvidiaExt.IncrementCounter());
    value.y = asfloat(g_NvidiaExt.IncrementCounter());
    value.z = asfloat(g_NvidiaExt.IncrementCounter());
    value.w = asfloat(g_NvidiaExt.IncrementCounter());
    return value;
}

int NvEvaluateAttributeSnappedForVPRS(int attrib, uint2 offset)
{
    int value = (int)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int2 NvEvaluateAttributeSnappedForVPRS(int2 attrib, uint2 offset)
{
    int2 value = (int2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int3 NvEvaluateAttributeSnappedForVPRS(int3 attrib, uint2 offset)
{
    int3 value = (int3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    value.z = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

int4 NvEvaluateAttributeSnappedForVPRS(int4 attrib, uint2 offset)
{
    int4 value = (int4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asint(g_NvidiaExt.IncrementCounter());
    value.y = asint(g_NvidiaExt.IncrementCounter());
    value.z = asint(g_NvidiaExt.IncrementCounter());
    value.w = asint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint NvEvaluateAttributeSnappedForVPRS(uint attrib, uint2 offset)
{
    uint value = (uint)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.x    = asuint(attrib.x);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 1;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint2 NvEvaluateAttributeSnappedForVPRS(uint2 attrib, uint2 offset)
{
    uint2 value = (uint2)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xy   = asuint(attrib.xy);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 2;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint3 NvEvaluateAttributeSnappedForVPRS(uint3 attrib, uint2 offset)
{
    uint3 value = (uint3)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyz  = asuint(attrib.xyz);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 3;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    value.z = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

uint4 NvEvaluateAttributeSnappedForVPRS(uint4 attrib, uint2 offset)
{
    uint4 value = (uint4)0;
    uint ext = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[ext].opcode     = NV_EXTN_OP_VPRS_EVAL_ATTRIB_SNAPPED;
    g_NvidiaExt[ext].src0u.xyzw = asuint(attrib.xyzw);
    g_NvidiaExt[ext].src1u.xy   = offset;
    g_NvidiaExt[ext].numOutputsForIncCounter = 4;
    value.x = asuint(g_NvidiaExt.IncrementCounter());
    value.y = asuint(g_NvidiaExt.IncrementCounter());
    value.z = asuint(g_NvidiaExt.IncrementCounter());
    value.w = asuint(g_NvidiaExt.IncrementCounter());
    return value;
}

// MATCH instruction variants 
uint NvWaveMatch(uint value)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x = value;
    g_NvidiaExt[index].src1u.x = 1;
    g_NvidiaExt[index].opcode  = NV_EXTN_OP_MATCH_ANY;
    // result is returned as the return value of IncrementCounter on fake UAV slot
    return g_NvidiaExt.IncrementCounter();
}

uint NvWaveMatch(uint2 value)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xy = value.xy;
    g_NvidiaExt[index].src1u.x = 2;
    g_NvidiaExt[index].opcode  = NV_EXTN_OP_MATCH_ANY;
    // result is returned as the return value of IncrementCounter on fake UAV slot
    return g_NvidiaExt.IncrementCounter();
}

uint NvWaveMatch(uint4 value)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u = value;
    g_NvidiaExt[index].src1u.x = 4;
    g_NvidiaExt[index].opcode  = NV_EXTN_OP_MATCH_ANY;
    // result is returned as the return value of IncrementCounter on fake UAV slot
    return g_NvidiaExt.IncrementCounter();
}

uint NvWaveMatch(float value)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x = asuint(value);
    g_NvidiaExt[index].src1u.x = 1;
    g_NvidiaExt[index].opcode  = NV_EXTN_OP_MATCH_ANY;
    // result is returned as the return value of IncrementCounter on fake UAV slot
    return g_NvidiaExt.IncrementCounter();
}

uint NvWaveMatch(float2 value)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xy = asuint(value);
    g_NvidiaExt[index].src1u.x = 2;
    g_NvidiaExt[index].opcode  = NV_EXTN_OP_MATCH_ANY;
    // result is returned as the return value of IncrementCounter on fake UAV slot
    return g_NvidiaExt.IncrementCounter();
}

uint NvWaveMatch(float4 value)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u = asuint(value);
    g_NvidiaExt[index].src1u.x = 4;
    g_NvidiaExt[index].opcode  = NV_EXTN_OP_MATCH_ANY;
    // result is returned as the return value of IncrementCounter on fake UAV slot
    return g_NvidiaExt.IncrementCounter();
}


//----------------------------------------------------------------------------//
//------------------------------ Footprint functions -------------------------//
//----------------------------------------------------------------------------//
// texSpace and smpSpace must be immediates, texIndex and smpIndex can be variable
// offset must be immediate
// the required components of location and offset fields can be filled depending on the dimension/type of the texture
// texType should be one of 2D or 3D as defined in nvShaderExtnEnums.h and and should be an immediate literal
// if the above restrictions are not met, the behaviour of this instruction is undefined

uint4 NvFootprintFine(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, int3 offset = int3(0, 0, 0))
{
    return __NvFootprint(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_FINE, gran, offset);
}

uint4 NvFootprintCoarse(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, int3 offset = int3(0, 0, 0))
{
    return __NvFootprint(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_COARSE, gran, offset);
}



uint4 NvFootprintFineBias(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float bias, int3 offset = int3(0, 0, 0))
{
    return __NvFootprintBias(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_FINE, gran, bias, offset);
}

uint4 NvFootprintCoarseBias(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float bias, int3 offset = int3(0, 0, 0))
{
    return __NvFootprintBias(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_COARSE, gran, bias, offset);
}



uint4 NvFootprintFineLevel(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float lodLevel, int3 offset = int3(0, 0, 0))
{
    return __NvFootprintLevel(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_FINE, gran, lodLevel, offset);
}

uint4 NvFootprintCoarseLevel(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float lodLevel, int3 offset = int3(0, 0, 0))
{
    return __NvFootprintLevel(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_COARSE, gran, lodLevel, offset);
}



uint4 NvFootprintFineGrad(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float3 ddx, float3 ddy, int3 offset = int3(0, 0, 0))
{
    return __NvFootprintGrad(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_FINE, gran, ddx, ddy, offset);
}

uint4 NvFootprintCoarseGrad(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float3 ddx, float3 ddy, int3 offset = int3(0, 0, 0))
{
    return __NvFootprintGrad(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_COARSE, gran, ddx, ddy, offset);
}

uint NvFootprintExtractLOD(uint4 blob)
{
    return ((blob.w & 0xF000) >> 12);
}

uint NvFootprintExtractReturnGran(uint4 blob)
{
    return ((blob.z & 0xF000000) >> 24);
}

uint2 NvFootprintExtractAnchorTileLoc2D(uint4 blob)
{
    uint2 loc;
    loc.x = (blob.w & 0xFFF);
    loc.y = (blob.z & 0xFFF);
    return loc;
}

uint3 NvFootprintExtractAnchorTileLoc3D(uint4 blob)
{
    uint3 loc;
    loc.x = (blob.w & 0xFFF);
    loc.y = ((blob.w & 0xFFF0000) >> 16);
    loc.z = (blob.z & 0x1FFF);
    return loc;
}

uint2 NvFootprintExtractOffset2D(uint4 blob)
{
    uint2 loc;
    loc.x = ((blob.z & 0x070000) >> 16);
    loc.y = ((blob.z & 0x380000) >> 19);
    return loc;
}

uint3 NvFootprintExtractOffset3D(uint4 blob)
{
    uint3 loc;
    loc.x = ((blob.z & 0x030000) >> 16);
    loc.y = ((blob.z & 0x0C0000) >> 18);
    loc.z = ((blob.z & 0x300000) >> 20);
    return loc;
}

uint2 NvFootprintExtractBitmask(uint4 blob)
{
    return blob.xy;
}


// Variant of Footprint extensions which returns isSingleLod (out parameter) 
// isSingleLod = true -> This footprint request touched the texels from only single LOD.
uint4 NvFootprintFine(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, out uint isSingleLod, int3 offset = int3(0, 0, 0))
{
    uint4 res = __NvFootprint(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_FINE, gran, offset);
    isSingleLod = __NvGetSpecial(NV_SPECIALOP_FOOTPRINT_SINGLELOD_PRED);
    return res;
}

uint4 NvFootprintCoarse(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, out uint isSingleLod, int3 offset = int3(0, 0, 0))
{
    uint4 res = __NvFootprint(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_COARSE, gran, offset);
    isSingleLod = __NvGetSpecial(NV_SPECIALOP_FOOTPRINT_SINGLELOD_PRED);
    return res;
}



uint4 NvFootprintFineBias(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float bias, out uint isSingleLod, int3 offset = int3(0, 0, 0))
{
    uint4 res = __NvFootprintBias(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_FINE, gran, bias, offset);
    isSingleLod = __NvGetSpecial(NV_SPECIALOP_FOOTPRINT_SINGLELOD_PRED);
    return res;
}

uint4 NvFootprintCoarseBias(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float bias, out uint isSingleLod, int3 offset = int3(0, 0, 0))
{
    uint4 res = __NvFootprintBias(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_COARSE, gran, bias, offset);
    isSingleLod = __NvGetSpecial(NV_SPECIALOP_FOOTPRINT_SINGLELOD_PRED);
    return res;
}



uint4 NvFootprintFineLevel(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float lodLevel, out uint isSingleLod, int3 offset = int3(0, 0, 0))
{
    uint4 res = __NvFootprintLevel(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_FINE, gran, lodLevel, offset);
    isSingleLod = __NvGetSpecial(NV_SPECIALOP_FOOTPRINT_SINGLELOD_PRED);
    return res;
}

uint4 NvFootprintCoarseLevel(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float lodLevel, out uint isSingleLod, int3 offset = int3(0, 0, 0))
{
    uint4 res = __NvFootprintLevel(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_COARSE, gran, lodLevel, offset);
    isSingleLod = __NvGetSpecial(NV_SPECIALOP_FOOTPRINT_SINGLELOD_PRED);
    return res;
}



uint4 NvFootprintFineGrad(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float3 ddx, float3 ddy, out uint isSingleLod, int3 offset = int3(0, 0, 0))
{
    uint4 res = __NvFootprintGrad(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_FINE, gran, ddx, ddy, offset);
    isSingleLod = __NvGetSpecial(NV_SPECIALOP_FOOTPRINT_SINGLELOD_PRED);
    return res;
}

uint4 NvFootprintCoarseGrad(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint gran, float3 ddx, float3 ddy, out uint isSingleLod, int3 offset = int3(0, 0, 0))
{
    uint4 res = __NvFootprintGrad(texSpace, texIndex, smpSpace, smpIndex, texType, location, NV_EXTN_FOOTPRINT_MODE_COARSE, gran, ddx, ddy, offset);
    isSingleLod = __NvGetSpecial(NV_SPECIALOP_FOOTPRINT_SINGLELOD_PRED);
    return res;
}


uint NvActiveThreads()
{
    return NvBallot(1);
}


//----------------------------------------------------------------------------//
//------------------------------ WaveMultiPrefix functions -------------------//
//----------------------------------------------------------------------------//

// Following are the WaveMultiPrefix functions for different operations (Add, Bitand, BitOr, BitXOr) for different datatypes (uint, uint2, uint4) 
// This is a set of functions which implement multi-prefix operations among the set of active lanes in the current wave (WARP). 
// A multi-prefix operation comprises a set of prefix operations, executed in parallel within subsets of lanes identified with the provided bitmasks. 
// These bitmasks represent partitioning of the set of active lanes in the current wave into N groups (where N is the number of unique masks across all lanes in the wave). 
// N prefix operations are then performed each within its corresponding group. 
// The groups are assumed to be non-intersecting (that is, a given lane can be a member of one and only one group), 
// and bitmasks in all lanes belonging to the same group are required to be the same.
// There are 2 type of functions - Exclusive and Inclusive prefix operations.
// e.g. For NvWaveMultiPrefixInclusiveAdd(val, mask) operation - For each of the groups (for which mask input is same) following is the expected output : 
// i^th thread in a group has value = sum(values of threads 0 to i)
// For Exclusive version of same opeartion - 
// i^th thread in a group has value = sum(values of threads 0 to i-1)  and 0th thread in a the Group has value 0 

// Extensions for Add 
uint NvWaveMultiPrefixInclusiveAdd(uint val, uint mask)
{
    uint temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        // As remainingThreads only has threads in group with smaller thread ids than its own thread-id nextLane can never be 31 for any thread in the group except the smallest one
        // For smallest thread in the group, remainingThreads is 0 -->  nextLane is ~0 (i.e. considering last 5 bits its 31)
        // So passing maskClampValue=30 to __NvShflGeneric, it will return laneValid=false for the smallest thread in the group. So update val and nextLane based on laneValid.
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val + temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint NvWaveMultiPrefixExclusiveAdd(uint val, uint mask)
{
    uint temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : 0;
    return NvWaveMultiPrefixInclusiveAdd(val, mask);
}

uint2 NvWaveMultiPrefixInclusiveAdd(uint2 val, uint mask)
{
    uint2 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val + temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint2 NvWaveMultiPrefixExclusiveAdd(uint2 val, uint mask)
{
    uint2 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : uint2(0, 0);
    return NvWaveMultiPrefixInclusiveAdd(val, mask);
}

uint4 NvWaveMultiPrefixInclusiveAdd(uint4 val, uint mask)
{
    uint4 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val + temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint4 NvWaveMultiPrefixExclusiveAdd(uint4 val, uint mask)
{
    uint4 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : uint4(0, 0, 0, 0);
    return NvWaveMultiPrefixInclusiveAdd(val, mask);
}

// MultiPrefix extensions for Bitand
uint NvWaveMultiPrefixInclusiveAnd(uint val, uint mask)
{
    uint temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val & temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint NvWaveMultiPrefixExclusiveAnd(uint val, uint mask)
{
    uint temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : ~0;
    return NvWaveMultiPrefixInclusiveAnd(val, mask);
}

uint2 NvWaveMultiPrefixInclusiveAnd(uint2 val, uint mask)
{
    uint2 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val & temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint2 NvWaveMultiPrefixExclusiveAnd(uint2 val, uint mask)
{
    uint2 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : uint2(~0, ~0);
    return NvWaveMultiPrefixInclusiveAnd(val, mask);
}


uint4 NvWaveMultiPrefixInclusiveAnd(uint4 val, uint mask)
{
    uint4 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val & temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint4 NvWaveMultiPrefixExclusiveAnd(uint4 val, uint mask)
{
    uint4 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : uint4(~0, ~0, ~0, ~0);
    return NvWaveMultiPrefixInclusiveAnd(val, mask);
}


// MultiPrefix extensions for BitOr
uint NvWaveMultiPrefixInclusiveOr(uint val, uint mask)
{
    uint temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val | temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint NvWaveMultiPrefixExclusiveOr(uint val, uint mask)
{
    uint temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : 0;
    return NvWaveMultiPrefixInclusiveOr(val, mask);
}

uint2 NvWaveMultiPrefixInclusiveOr(uint2 val, uint mask)
{
    uint2 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val | temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint2 NvWaveMultiPrefixExclusiveOr(uint2 val, uint mask)
{
    uint2 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : uint2(0, 0);
    return NvWaveMultiPrefixInclusiveOr(val, mask);
}


uint4 NvWaveMultiPrefixInclusiveOr(uint4 val, uint mask)
{
    uint4 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val | temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint4 NvWaveMultiPrefixExclusiveOr(uint4 val, uint mask)
{
    uint4 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : uint4(0, 0, 0, 0);
    return NvWaveMultiPrefixInclusiveOr(val, mask);
}


// MultiPrefix extensions for BitXOr
uint NvWaveMultiPrefixInclusiveXOr(uint val, uint mask)
{
    uint temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val ^ temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint NvWaveMultiPrefixExclusiveXOr(uint val, uint mask)
{
    uint temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : 0;
    return NvWaveMultiPrefixInclusiveXOr(val, mask);
}

uint2 NvWaveMultiPrefixInclusiveXOr(uint2 val, uint mask)
{
    uint2 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val ^ temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint2 NvWaveMultiPrefixExclusiveXOr(uint2 val, uint mask)
{
    uint2 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : uint2(0, 0);
    return NvWaveMultiPrefixInclusiveXOr(val, mask);
}


uint4 NvWaveMultiPrefixInclusiveXOr(uint4 val, uint mask)
{
    uint4 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint nextLane = firstbithigh(remainingThreads);
    for (uint i = 0; i < NV_WARP_SIZE_LOG2; i++)
    {
        temp = NvShfl(val, nextLane);
        uint laneValid;
        uint newLane = asuint(__NvShflGeneric(nextLane, nextLane, 30, laneValid));
        if (laneValid) // if nextLane's nextLane is valid
        {
            val = val ^ temp;
            nextLane = newLane;
        }
    }
    return val;
}

uint4 NvWaveMultiPrefixExclusiveXOr(uint4 val, uint mask)
{
    uint4 temp;
    uint a = NvActiveThreads();
    uint remainingThreads = a & __NvGetSpecial(NV_SPECIALOP_THREADLTMASK) & mask;
    uint lane = firstbithigh(remainingThreads);
    temp = NvShfl(val, lane);
    val = remainingThreads != 0 ? temp : uint4(0, 0, 0, 0);
    return NvWaveMultiPrefixInclusiveXOr(val, mask);
}


//----------------------------------------------------------------------------//
//------------------------- DXR Micro-map Extension --------------------------//
//----------------------------------------------------------------------------//

float3x3 NvRtTriangleObjectPositions()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_TRIANGLE_OBJECT_POSITIONS;

    float3x3 ret;
    ret[0][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[0][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[0][2] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][2] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2][2] = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

float3x3 NvRtMicroTriangleObjectPositions()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_MICRO_TRIANGLE_OBJECT_POSITIONS;

    float3x3 ret;
    ret[0][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[0][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[0][2] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][2] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2][2] = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

float3x2 NvRtMicroTriangleBarycentrics()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_MICRO_TRIANGLE_BARYCENTRICS;

    float3x2 ret;
    ret[0][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[0][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2][1] = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

bool NvRtIsMicroTriangleHit()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_IS_MICRO_TRIANGLE_HIT;
    uint ret = g_NvidiaExt.IncrementCounter();
    return ret != 0;
}

bool NvRtIsBackFacing()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_IS_BACK_FACING;
    uint ret = g_NvidiaExt.IncrementCounter();
    return ret != 0;
}

#if __SHADER_TARGET_MAJOR > 6 || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 5)

float3 NvRtMicroVertexObjectPosition(RaytracingAccelerationStructure AccelerationStructure, uint InstanceIndex, uint GeometryIndex, uint PrimitiveIndex, uint2 UV)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_MICRO_VERTEX_OBJECT_POSITION;
    g_NvidiaExt[index].src0u.x = InstanceIndex;
    g_NvidiaExt[index].src0u.y = GeometryIndex;
    g_NvidiaExt[index].src0u.z = PrimitiveIndex;
    g_NvidiaExt[index].src0u.w = UV.x;
    g_NvidiaExt[index].src1u.x = UV.y;
    uint handle = g_NvidiaExt.IncrementCounter();
    float3 ret;
    ret.x = asfloat(g_NvidiaExt.IncrementCounter());
    ret.y = asfloat(g_NvidiaExt.IncrementCounter());
    ret.z = asfloat(g_NvidiaExt.IncrementCounter());

    RayQuery<0> rq;
    rq.TraceRayInline(AccelerationStructure, 0, handle, (RayDesc)0);

    return ret;
}

float2 NvRtMicroVertexBarycentrics(RaytracingAccelerationStructure AccelerationStructure, uint InstanceIndex, uint GeometryIndex, uint PrimitiveIndex, uint2 UV)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_MICRO_VERTEX_BARYCENTRICS;
    g_NvidiaExt[index].src0u.x = InstanceIndex;
    g_NvidiaExt[index].src0u.y = GeometryIndex;
    g_NvidiaExt[index].src0u.z = PrimitiveIndex;
    g_NvidiaExt[index].src0u.w = UV.x;
    g_NvidiaExt[index].src1u.x = UV.y;
    uint handle = g_NvidiaExt.IncrementCounter();
    float2 ret;
    ret.x = asfloat(g_NvidiaExt.IncrementCounter());
    ret.y = asfloat(g_NvidiaExt.IncrementCounter());

    RayQuery<0> rq;
    rq.TraceRayInline(AccelerationStructure, 0, handle, (RayDesc)0);

    return ret;
}

#endif

//----------------------------------------------------------------------------//
//--------------------- DXR Cluster Geometry Extension -----------------------//
//----------------------------------------------------------------------------//

#if __SHADER_TARGET_MAJOR > 6 || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 3)

uint NvRtGetClusterID()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_GET_CLUSTER_ID;
    return g_NvidiaExt.IncrementCounter();
}

#endif

#if __SHADER_TARGET_MAJOR > 6 || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 5)

#define NvRtGetCandidateClusterID(rq) __NvRtGetCandidateClusterID(rq.RayFlags())

#define NvRtGetCommittedClusterID(rq) __NvRtGetCommittedClusterID(rq.RayFlags())

#define NvRtCandidateTriangleObjectPositions(rq) __NvRtCandidateTriangleObjectPositions(rq.RayFlags())

#define NvRtCommittedTriangleObjectPositions(rq) __NvRtCommittedTriangleObjectPositions(rq.RayFlags())

#endif

//----------------------------------------------------------------------------//
//--------------------- DXR Linear Swept Sphere Extension --------------------//
//----------------------------------------------------------------------------//

#if __SHADER_TARGET_MAJOR > 6 || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 3)

float4 NvRtSphereObjectPositionAndRadius()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_SPHERE_OBJECT_POSITION_AND_RADIUS;

    float4 ret;
    ret[0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[3] = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

float2x4 NvRtLssObjectPositionsAndRadii()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_LSS_OBJECT_POSITIONS_AND_RADII;

    float2x4 ret;
    ret[0][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[0][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[0][2] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[0][3] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][2] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1][3] = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

bool NvRtIsSphereHit()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_IS_SPHERE_HIT;
    uint ret = g_NvidiaExt.IncrementCounter();
    return ret != 0;
}

bool NvRtIsLssHit()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_IS_LSS_HIT;
    uint ret = g_NvidiaExt.IncrementCounter();
    return ret != 0;
}

#endif

#if __SHADER_TARGET_MAJOR > 6 || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 5)

#define NvRtCandidateIsNonOpaqueSphere(rq) __NvRtCandidateIsNonOpaqueSphere(rq.RayFlags())

#define NvRtCandidateIsNonOpaqueLss(rq) __NvRtCandidateIsNonOpaqueLss(rq.RayFlags())

#define NvRtCandidateLssHitParameter(rq) __NvRtCandidateLssHitParameter(rq.RayFlags())

#define NvRtCandidateSphereObjectPositionAndRadius(rq) __NvRtCandidateSphereObjectPositionAndRadius(rq.RayFlags())

#define NvRtCandidateLssObjectPositionsAndRadii(rq) __NvRtCandidateLssObjectPositionsAndRadii(rq.RayFlags())

#define NvRtCandidateBuiltinPrimitiveRayT(rq) __NvRtCandidateBuiltinPrimitiveRayT(rq.RayFlags())

#define NvRtCommittedIsSphere(rq) __NvRtCommittedIsSphere(rq.RayFlags())

#define NvRtCommittedIsLss(rq) __NvRtCommittedIsLss(rq.RayFlags())

#define NvRtCommittedLssHitParameter(rq) __NvRtCommittedLssHitParameter(rq.RayFlags())

#define NvRtCommittedSphereObjectPositionAndRadius(rq) __NvRtCommittedSphereObjectPositionAndRadius(rq.RayFlags())

#define NvRtCommittedLssObjectPositionsAndRadii(rq) __NvRtCommittedLssObjectPositionsAndRadii(rq.RayFlags())

#define NvRtCommitNonOpaqueBuiltinPrimitiveHit(rq) __NvRtCommitNonOpaqueBuiltinPrimitiveHit(rq.RayFlags())

#endif

//----------------------------------------------------------------------------//
//------------------------- DXR HitObject Extension --------------------------//
//----------------------------------------------------------------------------//

// Support for templates in HLSL requires HLSL 2021+. When using dxc,
// use the -HV 2021 command line argument to enable these versions.
#if defined(__HLSL_VERSION) && (__HLSL_VERSION >= 2021) && !defined(NV_HITOBJECT_USE_MACRO_API)

struct NvHitObject {
    uint _handle;

    bool IsMiss()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_MISS;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    bool IsHit()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_HIT;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    bool IsNop()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_NOP;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    uint GetInstanceID()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_ID;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint GetInstanceIndex()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_INDEX;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint GetPrimitiveIndex()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_PRIMITIVE_INDEX;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint GetGeometryIndex()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_GEOMETRY_INDEX;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint GetHitKind()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_HIT_KIND;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    RayDesc GetRayDesc()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_RAY_DESC;
        g_NvidiaExt[index].src0u.x = _handle;

        uint tmin = g_NvidiaExt.IncrementCounter();
        uint tmax = g_NvidiaExt.IncrementCounter();
        uint rayOrgX = g_NvidiaExt.IncrementCounter();
        uint rayOrgY = g_NvidiaExt.IncrementCounter();
        uint rayOrgZ = g_NvidiaExt.IncrementCounter();
        uint rayDirX = g_NvidiaExt.IncrementCounter();
        uint rayDirY = g_NvidiaExt.IncrementCounter();
        uint rayDirZ = g_NvidiaExt.IncrementCounter();

        RayDesc ray;
        ray.TMin = asfloat(tmin);
        ray.TMax = asfloat(tmax);
        ray.Origin.x = asfloat(rayOrgX);
        ray.Origin.y = asfloat(rayOrgY);
        ray.Origin.z = asfloat(rayOrgZ);
        ray.Direction.x = asfloat(rayDirX);
        ray.Direction.y = asfloat(rayDirY);
        ray.Direction.z = asfloat(rayDirZ);

        return ray;
    }

    template <typename T>
    T GetAttributes()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_ATTRIBUTES;
        g_NvidiaExt[index].src0u.x = _handle;
        uint callHandle = g_NvidiaExt.IncrementCounter();

        T attrs;
        CallShader(callHandle, attrs);
        return attrs;
    }

    uint GetShaderTableIndex()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_SHADER_TABLE_INDEX;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint LoadLocalRootTableConstant(uint RootConstantOffsetInBytes)
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_LOAD_LOCAL_ROOT_TABLE_CONSTANT;
        g_NvidiaExt[index].src0u.x = _handle;
        g_NvidiaExt[index].src0u.y = RootConstantOffsetInBytes;
        return g_NvidiaExt.IncrementCounter();
    }

    float4 GetSphereObjectPositionAndRadius()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_SPHERE_OBJECT_POSITION_AND_RADIUS;
        g_NvidiaExt[index].src0u.x = _handle;

        float4 ret;
        ret[0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[3] = asfloat(g_NvidiaExt.IncrementCounter());
        return ret;
    }

    float2x4 GetLssObjectPositionsAndRadii()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_LSS_OBJECT_POSITIONS_AND_RADII;
        g_NvidiaExt[index].src0u.x = _handle;

        float2x4 ret;
        ret[0][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][3] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][3] = asfloat(g_NvidiaExt.IncrementCounter());
        return ret;
    }

    bool IsSphereHit()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_SPHERE_HIT;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    bool IsLssHit()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_LSS_HIT;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    uint GetClusterID()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_CLUSTER_ID;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    float3x3 GetTriangleObjectPositions()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_TRIANGLE_OBJECT_POSITIONS;
        g_NvidiaExt[index].src0u.x = _handle;

        float3x3 ret;
        ret[0][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[2][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[2][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[2][2] = asfloat(g_NvidiaExt.IncrementCounter());
        return ret;
    }
};

template<typename T>
NvHitObject NvTraceRayHitObject(
    RaytracingAccelerationStructure AccelerationStructure,
    uint RayFlags,
    uint InstanceInclusionMask,
    uint RayContributionToHitGroupIndex,
    uint MultiplierForGeometryContributionToHitGroupIndex,
    uint MissShaderIndex,
    RayDesc Ray,
    inout T Payload)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_TRACE_RAY;
    g_NvidiaExt[index].numOutputsForIncCounter = 2;
    g_NvidiaExt[index].src0u.x = MissShaderIndex;
    uint hitHandle = g_NvidiaExt.IncrementCounter();
    uint traceHandle = g_NvidiaExt.IncrementCounter();

    TraceRay(AccelerationStructure, RayFlags, InstanceInclusionMask, RayContributionToHitGroupIndex, MultiplierForGeometryContributionToHitGroupIndex, traceHandle, Ray, Payload);

    NvHitObject hitObj;
    hitObj._handle = hitHandle;
    return hitObj;
}

template <typename T>
NvHitObject NvMakeHit(
    RaytracingAccelerationStructure AccelerationStructure,
    uint InstanceIndex,
    uint GeometryIndex,
    uint PrimitiveIndex,
    uint HitKind,
    uint RayContributionToHitGroupIndex,
    uint MultiplierForGeometryContributionToHitGroupIndex,
    RayDesc Ray,
    T Attributes)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_MAKE_HIT;
    g_NvidiaExt[index].numOutputsForIncCounter = 2;
    g_NvidiaExt[index].src0u.x = InstanceIndex;
    g_NvidiaExt[index].src0u.y = GeometryIndex;
    g_NvidiaExt[index].src0u.z = PrimitiveIndex;
    g_NvidiaExt[index].src0u.w = HitKind;
    g_NvidiaExt[index].src1u.x = RayContributionToHitGroupIndex;
    g_NvidiaExt[index].src1u.y = MultiplierForGeometryContributionToHitGroupIndex;
    uint hitHandle = g_NvidiaExt.IncrementCounter();
    uint traceHandle = g_NvidiaExt.IncrementCounter();

    struct AttrWrapper { T Attrs; };
    AttrWrapper wrapper;
    wrapper.Attrs = Attributes;
    CallShader(traceHandle, wrapper);

    struct DummyPayload { int a; };
    DummyPayload payload;
    TraceRay(AccelerationStructure, 0, 0, 0, 0, traceHandle, Ray, payload);

    NvHitObject hitObj;
    hitObj._handle = hitHandle;
    return hitObj;
}

template <typename T>
NvHitObject NvMakeHitWithRecordIndex(
    uint HitGroupRecordIndex,
    RaytracingAccelerationStructure AccelerationStructure,
    uint InstanceIndex,
    uint GeometryIndex,
    uint PrimitiveIndex,
    uint HitKind,
    RayDesc Ray,
    T Attributes)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_MAKE_HIT_WITH_RECORD_INDEX;
    g_NvidiaExt[index].numOutputsForIncCounter = 2;
    g_NvidiaExt[index].src0u.x = InstanceIndex;
    g_NvidiaExt[index].src0u.y = GeometryIndex;
    g_NvidiaExt[index].src0u.z = PrimitiveIndex;
    g_NvidiaExt[index].src0u.w = HitKind;
    g_NvidiaExt[index].src1u.x = HitGroupRecordIndex;
    uint hitHandle = g_NvidiaExt.IncrementCounter();
    uint traceHandle = g_NvidiaExt.IncrementCounter();

    struct AttrWrapper { T Attrs; };
    AttrWrapper wrapper;
    wrapper.Attrs = Attributes;
    CallShader(traceHandle, wrapper);

    struct DummyPayload { int a; };
    DummyPayload payload;
    TraceRay(AccelerationStructure, 0, 0, 0, 0, traceHandle, Ray, payload);

    NvHitObject hitObj;
    hitObj._handle = hitHandle;
    return hitObj;
}

NvHitObject NvMakeMiss(
    uint MissShaderIndex,
    RayDesc Ray)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_MAKE_MISS;
    g_NvidiaExt[index].src0u.x = MissShaderIndex;
    g_NvidiaExt[index].src0u.y = asuint(Ray.TMin);
    g_NvidiaExt[index].src0u.z = asuint(Ray.TMax);
    g_NvidiaExt[index].src1u.x = asuint(Ray.Origin.x);
    g_NvidiaExt[index].src1u.y = asuint(Ray.Origin.y);
    g_NvidiaExt[index].src1u.z = asuint(Ray.Origin.z);
    g_NvidiaExt[index].src2u.x = asuint(Ray.Direction.x);
    g_NvidiaExt[index].src2u.y = asuint(Ray.Direction.y);
    g_NvidiaExt[index].src2u.z = asuint(Ray.Direction.z);
    uint hitHandle = g_NvidiaExt.IncrementCounter();

    NvHitObject hitObj;
    hitObj._handle = hitHandle;
    return hitObj;
}

NvHitObject NvMakeNop()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_MAKE_NOP;
    uint hitHandle = g_NvidiaExt.IncrementCounter();

    NvHitObject hitObj;
    hitObj._handle = hitHandle;
    return hitObj;
}

void NvReorderThread(uint CoherenceHint, uint NumCoherenceHintBits)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_REORDER_THREAD;
    g_NvidiaExt[index].src0u.x = 0;
    g_NvidiaExt[index].src0u.y = 0;
    g_NvidiaExt[index].src0u.z = CoherenceHint;
    g_NvidiaExt[index].src0u.w = NumCoherenceHintBits;
    g_NvidiaExt.IncrementCounter();
}

void NvReorderThread(NvHitObject HitObj, uint CoherenceHint, uint NumCoherenceHintBits)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_REORDER_THREAD;
    g_NvidiaExt[index].src0u.x = 1;
    g_NvidiaExt[index].src0u.y = HitObj._handle;
    g_NvidiaExt[index].src0u.z = CoherenceHint;
    g_NvidiaExt[index].src0u.w = NumCoherenceHintBits;
    g_NvidiaExt.IncrementCounter();
}

void NvReorderThread(NvHitObject HitObj)
{
    NvReorderThread(HitObj, 0, 0);
}

template<typename T>
void NvInvokeHitObject(
    RaytracingAccelerationStructure AccelerationStructure,
    NvHitObject HitObj,
    inout T Payload)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_INVOKE;
    g_NvidiaExt[index].src0u.x = HitObj._handle;
    uint handle = g_NvidiaExt.IncrementCounter();

    TraceRay(AccelerationStructure, 0, 0, 0, 0, handle, (RayDesc)0, Payload);
}

// Macro-based version of the HitObject API. Use this when HLSL 2021 is not available.
// Enable by specifying #define NV_HITOBJECT_USE_MACRO_API before including this header.
#elif defined(NV_HITOBJECT_USE_MACRO_API)

struct NvHitObject {
    uint _handle;

    bool IsMiss()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_MISS;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    bool IsHit()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_HIT;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    bool IsNop()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_NOP;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    uint GetInstanceID()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_ID;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint GetInstanceIndex()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_INSTANCE_INDEX;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint GetPrimitiveIndex()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_PRIMITIVE_INDEX;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint GetGeometryIndex()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_GEOMETRY_INDEX;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint GetHitKind()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_HIT_KIND;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    RayDesc GetRayDesc()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_RAY_DESC;
        g_NvidiaExt[index].src0u.x = _handle;

        uint tmin = g_NvidiaExt.IncrementCounter();
        uint tmax = g_NvidiaExt.IncrementCounter();
        uint rayOrgX = g_NvidiaExt.IncrementCounter();
        uint rayOrgY = g_NvidiaExt.IncrementCounter();
        uint rayOrgZ = g_NvidiaExt.IncrementCounter();
        uint rayDirX = g_NvidiaExt.IncrementCounter();
        uint rayDirY = g_NvidiaExt.IncrementCounter();
        uint rayDirZ = g_NvidiaExt.IncrementCounter();

        RayDesc ray;
        ray.TMin = asfloat(tmin);
        ray.TMax = asfloat(tmax);
        ray.Origin.x = asfloat(rayOrgX);
        ray.Origin.y = asfloat(rayOrgY);
        ray.Origin.z = asfloat(rayOrgZ);
        ray.Direction.x = asfloat(rayDirX);
        ray.Direction.y = asfloat(rayDirY);
        ray.Direction.z = asfloat(rayDirZ);

        return ray;
    }    

    uint GetShaderTableIndex()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_SHADER_TABLE_INDEX;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    uint LoadLocalRootTableConstant(uint RootConstantOffsetInBytes)
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_LOAD_LOCAL_ROOT_TABLE_CONSTANT;
        g_NvidiaExt[index].src0u.x = _handle;
        g_NvidiaExt[index].src0u.y = RootConstantOffsetInBytes;
        return g_NvidiaExt.IncrementCounter();
    }

    float4 GetSphereObjectPositionAndRadius()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_SPHERE_OBJECT_POSITION_AND_RADIUS;
        g_NvidiaExt[index].src0u.x = _handle;

        float4 ret;
        ret[0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[3] = asfloat(g_NvidiaExt.IncrementCounter());
        return ret;
    }

    float2x4 GetLssObjectPositionsAndRadii()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_LSS_OBJECT_POSITIONS_AND_RADII;
        g_NvidiaExt[index].src0u.x = _handle;

        float2x4 ret;
        ret[0][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][3] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][3] = asfloat(g_NvidiaExt.IncrementCounter());
        return ret;
    }

    bool IsSphereHit()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_SPHERE_HIT;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    bool IsLssHit()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_IS_LSS_HIT;
        g_NvidiaExt[index].src0u.x = _handle;
        uint ret = g_NvidiaExt.IncrementCounter();
        return ret != 0;
    }

    uint GetClusterID()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_CLUSTER_ID;
        g_NvidiaExt[index].src0u.x = _handle;
        return g_NvidiaExt.IncrementCounter();
    }

    float3x3 GetTriangleObjectPositions()
    {
        uint index = g_NvidiaExt.IncrementCounter();
        g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_TRIANGLE_OBJECT_POSITIONS;
        g_NvidiaExt[index].src0u.x = _handle;

        float3x3 ret;
        ret[0][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[0][2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[1][2] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[2][0] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[2][1] = asfloat(g_NvidiaExt.IncrementCounter());
        ret[2][2] = asfloat(g_NvidiaExt.IncrementCounter());
        return ret;
    }
};

#define NvTraceRayHitObject(AccelerationStructure,RayFlags,InstanceInclusionMask,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToHitGroupIndex,MissShaderIndex,Ray,Payload,ResultHitObj) \
do { \
    uint _rayFlags = RayFlags; \
    uint _instanceInclusionMask = InstanceInclusionMask; \
    uint _rayContributionToHitGroupIndex = RayContributionToHitGroupIndex; \
    uint _multiplierForGeometryContributionToHitGroupIndex = MultiplierForGeometryContributionToHitGroupIndex; \
    uint _missShaderIndex = MissShaderIndex; \
    RayDesc _ray = Ray; \
    uint _index = g_NvidiaExt.IncrementCounter(); \
    g_NvidiaExt[_index].opcode = NV_EXTN_OP_HIT_OBJECT_TRACE_RAY; \
    g_NvidiaExt[_index].numOutputsForIncCounter = 2; \
    g_NvidiaExt[_index].src0u.x = _missShaderIndex; \
    uint _hitHandle = g_NvidiaExt.IncrementCounter(); \
    uint _traceHandle = g_NvidiaExt.IncrementCounter(); \
    TraceRay(AccelerationStructure, _rayFlags, _instanceInclusionMask, _rayContributionToHitGroupIndex, _multiplierForGeometryContributionToHitGroupIndex, _traceHandle, _ray, Payload); \
    ResultHitObj._handle = _hitHandle; \
} while(0)

struct NvHitObjectMacroDummyPayloadType { int a; };

#define NvMakeHit(AccelerationStructure,InstanceIndex,GeometryIndex,PrimitiveIndex,HitKind,RayContributionToHitGroupIndex,MultiplierForGeometryContributionToHitGroupIndex,Ray,Attributes,ResultHitObj) \
do { \
    uint _instanceIndex = InstanceIndex; \
    uint _geometryIndex = GeometryIndex; \
    uint _primitiveIndex = PrimitiveIndex; \
    uint _hitKind = HitKind; \
    uint _rayContributionToHitGroupIndex = RayContributionToHitGroupIndex; \
    uint _multiplierForGeometryContributionToHitGroupIndex = MultiplierForGeometryContributionToHitGroupIndex; \
    RayDesc _ray = Ray; \
    uint _index = g_NvidiaExt.IncrementCounter(); \
    g_NvidiaExt[_index].opcode = NV_EXTN_OP_HIT_OBJECT_MAKE_HIT; \
    g_NvidiaExt[_index].numOutputsForIncCounter = 2; \
    g_NvidiaExt[_index].src0u.x = _instanceIndex; \
    g_NvidiaExt[_index].src0u.y = _geometryIndex; \
    g_NvidiaExt[_index].src0u.z = _primitiveIndex; \
    g_NvidiaExt[_index].src0u.w = _hitKind; \
    g_NvidiaExt[_index].src1u.x = _rayContributionToHitGroupIndex; \
    g_NvidiaExt[_index].src1u.y = _multiplierForGeometryContributionToHitGroupIndex; \
    uint _hitHandle = g_NvidiaExt.IncrementCounter(); \
    uint _traceHandle = g_NvidiaExt.IncrementCounter(); \
    CallShader(_traceHandle, Attributes); \
    NvHitObjectMacroDummyPayloadType _payload; \
    TraceRay(AccelerationStructure, 0, 0, 0, 0, _traceHandle, _ray, _payload); \
    ResultHitObj._handle = _hitHandle; \
} while(0)

#define NvMakeHitWithRecordIndex(HitGroupRecordIndex,AccelerationStructure,InstanceIndex,GeometryIndex,PrimitiveIndex,HitKind,Ray,Attributes,ResultHitObj) \
do { \
    uint _hitGroupRecordIndex = HitGroupRecordIndex; \
    uint _instanceIndex = InstanceIndex; \
    uint _geometryIndex = GeometryIndex; \
    uint _primitiveIndex = PrimitiveIndex; \
    uint _hitKind = HitKind; \
    RayDesc _ray = Ray; \
    uint _index = g_NvidiaExt.IncrementCounter(); \
    g_NvidiaExt[_index].opcode = NV_EXTN_OP_HIT_OBJECT_MAKE_HIT_WITH_RECORD_INDEX; \
    g_NvidiaExt[_index].numOutputsForIncCounter = 2; \
    g_NvidiaExt[_index].src0u.x = _instanceIndex; \
    g_NvidiaExt[_index].src0u.y = _geometryIndex; \
    g_NvidiaExt[_index].src0u.z = _primitiveIndex; \
    g_NvidiaExt[_index].src0u.w = _hitKind; \
    g_NvidiaExt[_index].src1u.x = _hitGroupRecordIndex; \
    uint _hitHandle = g_NvidiaExt.IncrementCounter(); \
    uint _traceHandle = g_NvidiaExt.IncrementCounter(); \
    CallShader(_traceHandle, Attributes); \
    NvHitObjectMacroDummyPayloadType _payload; \
    TraceRay(AccelerationStructure, 0, 0, 0, 0, _traceHandle, _ray, _payload); \
    ResultHitObj._handle = _hitHandle; \
} while(0)

NvHitObject NvMakeMiss(
    uint MissShaderIndex,
    RayDesc Ray)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_MAKE_MISS;
    g_NvidiaExt[index].src0u.x = MissShaderIndex;
    g_NvidiaExt[index].src0u.y = asuint(Ray.TMin);
    g_NvidiaExt[index].src0u.z = asuint(Ray.TMax);
    g_NvidiaExt[index].src1u.x = asuint(Ray.Origin.x);
    g_NvidiaExt[index].src1u.y = asuint(Ray.Origin.y);
    g_NvidiaExt[index].src1u.z = asuint(Ray.Origin.z);
    g_NvidiaExt[index].src2u.x = asuint(Ray.Direction.x);
    g_NvidiaExt[index].src2u.y = asuint(Ray.Direction.y);
    g_NvidiaExt[index].src2u.z = asuint(Ray.Direction.z);
    uint hitHandle = g_NvidiaExt.IncrementCounter();

    NvHitObject hitObj;
    hitObj._handle = hitHandle;
    return hitObj;
}

NvHitObject NvMakeNop()
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_MAKE_NOP;
    uint hitHandle = g_NvidiaExt.IncrementCounter();

    NvHitObject hitObj;
    hitObj._handle = hitHandle;
    return hitObj;
}

#define NvGetAttributesFromHitObject(HitObj,ResultAttributes) \
do { \
    uint _index = g_NvidiaExt.IncrementCounter(); \
    g_NvidiaExt[_index].opcode = NV_EXTN_OP_HIT_OBJECT_GET_ATTRIBUTES; \
    g_NvidiaExt[_index].src0u.x = HitObj._handle; \
    uint _callHandle = g_NvidiaExt.IncrementCounter(); \
    CallShader(_callHandle, ResultAttributes); \
} while(0)

void NvReorderThread(uint CoherenceHint, uint NumCoherenceHintBits)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_REORDER_THREAD;
    g_NvidiaExt[index].src0u.x = 0;
    g_NvidiaExt[index].src0u.y = 0;
    g_NvidiaExt[index].src0u.z = CoherenceHint;
    g_NvidiaExt[index].src0u.w = NumCoherenceHintBits;
    g_NvidiaExt.IncrementCounter();
}

void NvReorderThread(NvHitObject HitObj, uint CoherenceHint, uint NumCoherenceHintBits)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_HIT_OBJECT_REORDER_THREAD;
    g_NvidiaExt[index].src0u.x = 1;
    g_NvidiaExt[index].src0u.y = HitObj._handle;
    g_NvidiaExt[index].src0u.z = CoherenceHint;
    g_NvidiaExt[index].src0u.w = NumCoherenceHintBits;
    g_NvidiaExt.IncrementCounter();
}

void NvReorderThread(NvHitObject HitObj)
{
    NvReorderThread(HitObj, 0, 0);
}

#define NvInvokeHitObject(AccelerationStructure,HitObj,Payload) \
do { \
    uint _index = g_NvidiaExt.IncrementCounter(); \
    g_NvidiaExt[_index].opcode = NV_EXTN_OP_HIT_OBJECT_INVOKE; \
    g_NvidiaExt[_index].src0u.x = HitObj._handle; \
    uint _handle = g_NvidiaExt.IncrementCounter(); \
    TraceRay(AccelerationStructure, 0, 0, 0, 0, _handle, (RayDesc)0, Payload); \
} while(0)

#endif
