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
// internal functions
// Functions in this file are not expected to be called by apps directly

#include "nvShaderExtnEnums.h"

struct NvShaderExtnStruct
{
    uint   opcode;                  // opcode
    uint   rid;                     // resource ID
    uint   sid;                     // sampler ID
            
    uint4  dst1u;                   // destination operand 1 (for instructions that need extra destination operands)
    uint4  src3u;                   // source operand 3
    uint4  src4u;                   // source operand 4
    uint4  src5u;                   // source operand 5
            
    uint4  src0u;                   // uint source operand  0
    uint4  src1u;                   // uint source operand  0
    uint4  src2u;                   // uint source operand  0
    uint4  dst0u;                   // uint destination operand
            
    uint   markUavRef;              // the next store to UAV is fake and is used only to identify the uav slot
    uint   numOutputsForIncCounter; // Used for output to IncrementCounter 
    float  padding1[27];            // struct size: 256 bytes
};

// RW structured buffer for Nvidia shader extensions

// Application needs to define NV_SHADER_EXTN_SLOT as a unused slot, which should be 
// set using NvAPI_D3D11_SetNvShaderExtnSlot() call before creating the first shader that
// uses nvidia shader extensions. E.g before including this file in shader define it as:
// #define NV_SHADER_EXTN_SLOT u7

// For SM5.1, application needs to define NV_SHADER_EXTN_REGISTER_SPACE as register space
// E.g. before including this file in shader define it as:
// #define NV_SHADER_EXTN_REGISTER_SPACE space2

// Note that other operations to this UAV will be ignored so application
// should bind a null resource

#ifdef NV_SHADER_EXTN_REGISTER_SPACE
RWStructuredBuffer<NvShaderExtnStruct> g_NvidiaExt : register( NV_SHADER_EXTN_SLOT, NV_SHADER_EXTN_REGISTER_SPACE );
#else
RWStructuredBuffer<NvShaderExtnStruct> g_NvidiaExt : register( NV_SHADER_EXTN_SLOT );
#endif

//----------------------------------------------------------------------------//
// the exposed SHFL instructions accept a mask parameter in src2
// To compute lane mask from width of segment:
// minLaneID : currentLaneId & src2[12:8]
// maxLaneID : minLaneId | (src2[4:0] & ~src2[12:8])
// where [minLaneId, maxLaneId] defines the segment where currentLaneId belongs
// we always set src2[4:0] to 11111 (0x1F), and set src2[12:8] as (32 - width)
int __NvGetShflMaskFromWidth(uint width)
{
    return ((NV_WARP_SIZE - width) << 8) | 0x1F;
}

//----------------------------------------------------------------------------//

void __NvReferenceUAVForOp(RWByteAddressBuffer uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav.Store(index, 0);
}

void __NvReferenceUAVForOp(RWTexture1D<float2> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[index] = float2(0,0);
}

void __NvReferenceUAVForOp(RWTexture2D<float2> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint2(index,index)] = float2(0,0);
}

void __NvReferenceUAVForOp(RWTexture3D<float2> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint3(index,index,index)] = float2(0,0);
}

void __NvReferenceUAVForOp(RWTexture1D<float4> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[index] = float4(0,0,0,0);
}

void __NvReferenceUAVForOp(RWTexture2D<float4> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint2(index,index)] = float4(0,0,0,0);
}

void __NvReferenceUAVForOp(RWTexture3D<float4> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint3(index,index,index)] = float4(0,0,0,0);
}

void __NvReferenceUAVForOp(RWTexture1D<float> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[index] = 0.0f;
}

void __NvReferenceUAVForOp(RWTexture2D<float> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint2(index,index)] = 0.0f;
}

void __NvReferenceUAVForOp(RWTexture3D<float> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint3(index,index,index)] = 0.0f;
}


void __NvReferenceUAVForOp(RWTexture1D<uint2> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[index] = uint2(0,0);
}

void __NvReferenceUAVForOp(RWTexture2D<uint2> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint2(index,index)] = uint2(0,0);
}

void __NvReferenceUAVForOp(RWTexture3D<uint2> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint3(index,index,index)] = uint2(0,0);
}

void __NvReferenceUAVForOp(RWTexture1D<uint4> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[index] = uint4(0,0,0,0);
}

void __NvReferenceUAVForOp(RWTexture2D<uint4> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint2(index,index)] = uint4(0,0,0,0);
}

void __NvReferenceUAVForOp(RWTexture3D<uint4> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint3(index,index,index)] = uint4(0,0,0,0);
}

void __NvReferenceUAVForOp(RWTexture1D<uint> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[index] = 0;
}

void __NvReferenceUAVForOp(RWTexture2D<uint> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint2(index,index)] = 0;
}

void __NvReferenceUAVForOp(RWTexture3D<uint> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint3(index,index,index)] = 0;
}

void __NvReferenceUAVForOp(RWTexture1D<int2> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[index] = int2(0,0);
}

void __NvReferenceUAVForOp(RWTexture2D<int2> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint2(index,index)] = int2(0,0);
}

void __NvReferenceUAVForOp(RWTexture3D<int2> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint3(index,index,index)] = int2(0,0);
}

void __NvReferenceUAVForOp(RWTexture1D<int4> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[index] = int4(0,0,0,0);
}

void __NvReferenceUAVForOp(RWTexture2D<int4> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint2(index,index)] = int4(0,0,0,0);
}

void __NvReferenceUAVForOp(RWTexture3D<int4> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint3(index,index,index)] = int4(0,0,0,0);
}

void __NvReferenceUAVForOp(RWTexture1D<int> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[index] = 0;
}

void __NvReferenceUAVForOp(RWTexture2D<int> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint2(index,index)] = 0;
}

void __NvReferenceUAVForOp(RWTexture3D<int> uav)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].markUavRef = 1;
    uav[uint3(index,index,index)] = 0;
}

//----------------------------------------------------------------------------//
// ATOMIC op sub-opcodes
#define NV_EXTN_ATOM_AND                            0
#define NV_EXTN_ATOM_OR                             1
#define NV_EXTN_ATOM_XOR                            2

#define NV_EXTN_ATOM_ADD                            3
#define NV_EXTN_ATOM_MAX                            6
#define NV_EXTN_ATOM_MIN                            7

#define NV_EXTN_ATOM_SWAP                           8
#define NV_EXTN_ATOM_CAS                            9

//----------------------------------------------------------------------------//

// performs Atomic operation on two consecutive fp16 values in the given UAV
// the uint paramater 'fp16x2Val' is treated as two fp16 values
// the passed sub-opcode 'op' should be an immediate constant
// byteAddress must be multiple of 4
// the returned value are the two fp16 values packed into a single uint
uint __NvAtomicOpFP16x2(RWByteAddressBuffer uav, uint byteAddress, uint fp16x2Val, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x = byteAddress;
    g_NvidiaExt[index].src1u.x = fp16x2Val;
    g_NvidiaExt[index].src2u.x = atomicOpType;
    g_NvidiaExt[index].opcode  = NV_EXTN_OP_FP16_ATOMIC;

    return g_NvidiaExt[index].dst0u.x;    
}

//----------------------------------------------------------------------------//

// performs Atomic operation on a R16G16_FLOAT UAV at the given address
// the uint paramater 'fp16x2Val' is treated as two fp16 values
// the passed sub-opcode 'op' should be an immediate constant
// the returned value are the two fp16 values (.x and .y components) packed into a single uint
// Warning: Behaviour of these set of functions is undefined if the UAV is not 
// of R16G16_FLOAT format (might result in app crash or TDR)

uint __NvAtomicOpFP16x2(RWTexture1D<float2> uav, uint address, uint fp16x2Val, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x    = address;
    g_NvidiaExt[index].src1u.x    = fp16x2Val;
    g_NvidiaExt[index].src2u.x    = atomicOpType;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP16_ATOMIC;

    return g_NvidiaExt[index].dst0u.x;    
}

uint __NvAtomicOpFP16x2(RWTexture2D<float2> uav, uint2 address, uint fp16x2Val, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xy   = address;
    g_NvidiaExt[index].src1u.x    = fp16x2Val;
    g_NvidiaExt[index].src2u.x    = atomicOpType;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP16_ATOMIC;

    return g_NvidiaExt[index].dst0u.x;    
}

uint __NvAtomicOpFP16x2(RWTexture3D<float2> uav, uint3 address, uint fp16x2Val, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xyz  = address;
    g_NvidiaExt[index].src1u.x    = fp16x2Val;
    g_NvidiaExt[index].src2u.x    = atomicOpType;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP16_ATOMIC;

    return g_NvidiaExt[index].dst0u.x;    
}

//----------------------------------------------------------------------------//

// performs Atomic operation on a R16G16B16A16_FLOAT UAV at the given address
// the uint2 paramater 'fp16x2Val' is treated as four fp16 values 
// i.e, fp16x2Val.x = uav.xy and fp16x2Val.y = uav.yz
// the passed sub-opcode 'op' should be an immediate constant
// the returned value are the four fp16 values (.xyzw components) packed into uint2
// Warning: Behaviour of these set of functions is undefined if the UAV is not 
// of R16G16B16A16_FLOAT format (might result in app crash or TDR)

uint2 __NvAtomicOpFP16x2(RWTexture1D<float4> uav, uint address, uint2 fp16x2Val, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);

    // break it down into two fp16x2 atomic ops
    uint2 retVal;

    // first op has x-coordinate = x * 2
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x    = address * 2;
    g_NvidiaExt[index].src1u.x    = fp16x2Val.x;
    g_NvidiaExt[index].src2u.x    = atomicOpType;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP16_ATOMIC;
    retVal.x = g_NvidiaExt[index].dst0u.x;

    // second op has x-coordinate = x * 2 + 1
    index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x    = address * 2 + 1;
    g_NvidiaExt[index].src1u.x    = fp16x2Val.y;
    g_NvidiaExt[index].src2u.x    = atomicOpType;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP16_ATOMIC;
    retVal.y = g_NvidiaExt[index].dst0u.x;

    return retVal;
}

uint2 __NvAtomicOpFP16x2(RWTexture2D<float4> uav, uint2 address, uint2 fp16x2Val, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);

    // break it down into two fp16x2 atomic ops
    uint2 retVal;

    // first op has x-coordinate = x * 2
    uint2 addressTemp = uint2(address.x * 2, address.y);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xy   = addressTemp;
    g_NvidiaExt[index].src1u.x    = fp16x2Val.x;
    g_NvidiaExt[index].src2u.x    = atomicOpType;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP16_ATOMIC;
    retVal.x = g_NvidiaExt[index].dst0u.x;

    // second op has x-coordinate = x * 2 + 1
    addressTemp.x++;
    index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xy   = addressTemp;
    g_NvidiaExt[index].src1u.x    = fp16x2Val.y;
    g_NvidiaExt[index].src2u.x    = atomicOpType;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP16_ATOMIC;
    retVal.y = g_NvidiaExt[index].dst0u.x;

    return retVal;
}

uint2 __NvAtomicOpFP16x2(RWTexture3D<float4> uav, uint3 address, uint2 fp16x2Val, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);

    // break it down into two fp16x2 atomic ops
    uint2 retVal;

    // first op has x-coordinate = x * 2
    uint3 addressTemp = uint3(address.x * 2, address.y, address.z);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xyz  = addressTemp;
    g_NvidiaExt[index].src1u.x    = fp16x2Val.x;
    g_NvidiaExt[index].src2u.x    = atomicOpType;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP16_ATOMIC;
    retVal.x = g_NvidiaExt[index].dst0u.x;

    // second op has x-coordinate = x * 2 + 1
    addressTemp.x++;
    index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xyz  = addressTemp;
    g_NvidiaExt[index].src1u.x    = fp16x2Val.y;
    g_NvidiaExt[index].src2u.x    = atomicOpType;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP16_ATOMIC;
    retVal.y = g_NvidiaExt[index].dst0u.x;

    return retVal;
}

uint __fp32x2Tofp16x2(float2 val)
{
    return (f32tof16(val.y)<<16) | f32tof16(val.x) ;
}

uint2 __fp32x4Tofp16x4(float4 val)
{
    return uint2( (f32tof16(val.y)<<16) | f32tof16(val.x), (f32tof16(val.w)<<16) | f32tof16(val.z) ) ;
}

//----------------------------------------------------------------------------//

// FP32 Atomic functions
// performs Atomic operation treating the uav as float (fp32) values
// the passed sub-opcode 'op' should be an immediate constant
// byteAddress must be multiple of 4
float __NvAtomicAddFP32(RWByteAddressBuffer uav, uint byteAddress, float val)
{
    __NvReferenceUAVForOp(uav);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x = byteAddress;
    g_NvidiaExt[index].src1u.x = asuint(val);   // passing as uint to make it more convinient for the driver to translate
    g_NvidiaExt[index].src2u.x = NV_EXTN_ATOM_ADD;
    g_NvidiaExt[index].opcode  = NV_EXTN_OP_FP32_ATOMIC;

    return asfloat(g_NvidiaExt[index].dst0u.x);
}

float __NvAtomicAddFP32(RWTexture1D<float> uav, uint address, float val)
{
    __NvReferenceUAVForOp(uav);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x    = address;
    g_NvidiaExt[index].src1u.x    = asuint(val);
    g_NvidiaExt[index].src2u.x    = NV_EXTN_ATOM_ADD;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP32_ATOMIC;

    return asfloat(g_NvidiaExt[index].dst0u.x);
}

float __NvAtomicAddFP32(RWTexture2D<float> uav, uint2 address, float val)
{
    __NvReferenceUAVForOp(uav);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xy   = address;
    g_NvidiaExt[index].src1u.x    = asuint(val);
    g_NvidiaExt[index].src2u.x    = NV_EXTN_ATOM_ADD;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP32_ATOMIC;

    return asfloat(g_NvidiaExt[index].dst0u.x);
}

float __NvAtomicAddFP32(RWTexture3D<float> uav, uint3 address, float val)
{
    __NvReferenceUAVForOp(uav);
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xyz  = address;
    g_NvidiaExt[index].src1u.x    = asuint(val);
    g_NvidiaExt[index].src2u.x    = NV_EXTN_ATOM_ADD;
    g_NvidiaExt[index].opcode     = NV_EXTN_OP_FP32_ATOMIC;

    return asfloat(g_NvidiaExt[index].dst0u.x);
}

//----------------------------------------------------------------------------//

// UINT64 Atmoic Functions
// The functions below performs atomic operation on the given UAV treating the value as uint64
// byteAddress must be multiple of 8
// The returned value is the value present in memory location before the atomic operation
// uint2 vector type is used to represent a single uint64 value with the x component containing the low 32 bits and y component the high 32 bits.

uint2 __NvAtomicCompareExchangeUINT64(RWByteAddressBuffer uav, uint byteAddress, uint2 compareValue, uint2 value)
{
    __NvReferenceUAVForOp(uav);

    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  = byteAddress;
    g_NvidiaExt[index].src1u.xy = compareValue;
    g_NvidiaExt[index].src1u.zw = value;
    g_NvidiaExt[index].src2u.x  = NV_EXTN_ATOM_CAS;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_UINT64_ATOMIC;

    return g_NvidiaExt[index].dst0u.xy;
}

uint2 __NvAtomicOpUINT64(RWByteAddressBuffer uav, uint byteAddress, uint2 value, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);

    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  = byteAddress;
    g_NvidiaExt[index].src1u.xy = value;
    g_NvidiaExt[index].src2u.x  = atomicOpType;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_UINT64_ATOMIC;

    return g_NvidiaExt[index].dst0u.xy;
}

uint2 __NvAtomicCompareExchangeUINT64(RWTexture1D<uint2> uav, uint address, uint2 compareValue, uint2 value)
{
    __NvReferenceUAVForOp(uav);

    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  = address;
    g_NvidiaExt[index].src1u.xy = compareValue;
    g_NvidiaExt[index].src1u.zw = value;
    g_NvidiaExt[index].src2u.x  = NV_EXTN_ATOM_CAS;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_UINT64_ATOMIC;

    return g_NvidiaExt[index].dst0u.xy;
}

uint2 __NvAtomicOpUINT64(RWTexture1D<uint2> uav, uint address, uint2 value, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);

    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  = address;
    g_NvidiaExt[index].src1u.xy = value;
    g_NvidiaExt[index].src2u.x  = atomicOpType;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_UINT64_ATOMIC;

    return g_NvidiaExt[index].dst0u.xy;
}

uint2 __NvAtomicCompareExchangeUINT64(RWTexture2D<uint2> uav, uint2 address, uint2 compareValue, uint2 value)
{
    __NvReferenceUAVForOp(uav);

    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xy  = address;
    g_NvidiaExt[index].src1u.xy = compareValue;
    g_NvidiaExt[index].src1u.zw = value;
    g_NvidiaExt[index].src2u.x  = NV_EXTN_ATOM_CAS;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_UINT64_ATOMIC;

    return g_NvidiaExt[index].dst0u.xy;
}

uint2 __NvAtomicOpUINT64(RWTexture2D<uint2> uav, uint2 address, uint2 value, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);

    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xy  = address;
    g_NvidiaExt[index].src1u.xy = value;
    g_NvidiaExt[index].src2u.x  = atomicOpType;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_UINT64_ATOMIC;

    return g_NvidiaExt[index].dst0u.xy;
}

uint2 __NvAtomicCompareExchangeUINT64(RWTexture3D<uint2> uav, uint3 address, uint2 compareValue, uint2 value)
{
    __NvReferenceUAVForOp(uav);

    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xyz  = address;
    g_NvidiaExt[index].src1u.xy = compareValue;
    g_NvidiaExt[index].src1u.zw = value;
    g_NvidiaExt[index].src2u.x  = NV_EXTN_ATOM_CAS;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_UINT64_ATOMIC;

    return g_NvidiaExt[index].dst0u.xy;
}

uint2 __NvAtomicOpUINT64(RWTexture3D<uint2> uav, uint3 address, uint2 value, uint atomicOpType)
{
    __NvReferenceUAVForOp(uav);

    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.xyz  = address;
    g_NvidiaExt[index].src1u.xy = value;
    g_NvidiaExt[index].src2u.x  = atomicOpType;
    g_NvidiaExt[index].opcode   = NV_EXTN_OP_UINT64_ATOMIC;

    return g_NvidiaExt[index].dst0u.xy;
}


uint4 __NvFootprint(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint footprintmode, uint gran, int3 offset = int3(0, 0, 0))
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x = texIndex;
    g_NvidiaExt[index].src0u.y  = smpIndex;
    g_NvidiaExt[index].src1u.xyz = asuint(location);
    g_NvidiaExt[index].src1u.w = gran;
    g_NvidiaExt[index].src3u.x = texSpace;
    g_NvidiaExt[index].src3u.y = smpSpace;
    g_NvidiaExt[index].src3u.z = texType;
    g_NvidiaExt[index].src3u.w = footprintmode;
    g_NvidiaExt[index].src4u.xyz = asuint(offset);

    g_NvidiaExt[index].opcode = NV_EXTN_OP_FOOTPRINT;
    g_NvidiaExt[index].numOutputsForIncCounter = 4;

    // result is returned as the return value of IncrementCounter on fake UAV slot
    uint4 op;
    op.x = g_NvidiaExt.IncrementCounter();
    op.y = g_NvidiaExt.IncrementCounter();
    op.z = g_NvidiaExt.IncrementCounter();
    op.w = g_NvidiaExt.IncrementCounter();
    return op;
}

uint4 __NvFootprintBias(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint footprintmode, uint gran, float bias, int3 offset = int3(0, 0, 0))
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x = texIndex;
    g_NvidiaExt[index].src0u.y  = smpIndex;
    g_NvidiaExt[index].src1u.xyz = asuint(location);
    g_NvidiaExt[index].src1u.w = gran;
    g_NvidiaExt[index].src2u.x = asuint(bias);
    g_NvidiaExt[index].src3u.x = texSpace;
    g_NvidiaExt[index].src3u.y = smpSpace;
    g_NvidiaExt[index].src3u.z = texType;
    g_NvidiaExt[index].src3u.w = footprintmode;
    g_NvidiaExt[index].src4u.xyz = asuint(offset);

    g_NvidiaExt[index].opcode = NV_EXTN_OP_FOOTPRINT_BIAS;
    g_NvidiaExt[index].numOutputsForIncCounter = 4;

    // result is returned as the return value of IncrementCounter on fake UAV slot
    uint4 op;
    op.x = g_NvidiaExt.IncrementCounter();
    op.y = g_NvidiaExt.IncrementCounter();
    op.z = g_NvidiaExt.IncrementCounter();
    op.w = g_NvidiaExt.IncrementCounter();
    return op;
}

uint4 __NvFootprintLevel(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint footprintmode, uint gran, float lodLevel, int3 offset = int3(0, 0, 0))
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x = texIndex;
    g_NvidiaExt[index].src0u.y  = smpIndex;
    g_NvidiaExt[index].src1u.xyz = asuint(location);
    g_NvidiaExt[index].src1u.w = gran;
    g_NvidiaExt[index].src2u.x = asuint(lodLevel);
    g_NvidiaExt[index].src3u.x = texSpace;
    g_NvidiaExt[index].src3u.y = smpSpace;
    g_NvidiaExt[index].src3u.z = texType;
    g_NvidiaExt[index].src3u.w = footprintmode;
    g_NvidiaExt[index].src4u.xyz = asuint(offset);

    g_NvidiaExt[index].opcode = NV_EXTN_OP_FOOTPRINT_LEVEL;
    g_NvidiaExt[index].numOutputsForIncCounter = 4;

    // result is returned as the return value of IncrementCounter on fake UAV slot
    uint4 op;
    op.x = g_NvidiaExt.IncrementCounter();
    op.y = g_NvidiaExt.IncrementCounter();
    op.z = g_NvidiaExt.IncrementCounter();
    op.w = g_NvidiaExt.IncrementCounter();
    return op;
}

uint4 __NvFootprintGrad(uint texSpace, uint texIndex, uint smpSpace, uint smpIndex, uint texType, float3 location, uint footprintmode, uint gran, float3 ddx, float3 ddy, int3 offset = int3(0, 0, 0))
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x = texIndex;
    g_NvidiaExt[index].src0u.y  = smpIndex;
    g_NvidiaExt[index].src1u.xyz = asuint(location);
    g_NvidiaExt[index].src1u.w = gran;
    g_NvidiaExt[index].src2u.xyz = asuint(ddx);
    g_NvidiaExt[index].src5u.xyz = asuint(ddy);
    g_NvidiaExt[index].src3u.x = texSpace;
    g_NvidiaExt[index].src3u.y = smpSpace;
    g_NvidiaExt[index].src3u.z = texType;
    g_NvidiaExt[index].src3u.w = footprintmode;
    g_NvidiaExt[index].src4u.xyz = asuint(offset);
    g_NvidiaExt[index].opcode = NV_EXTN_OP_FOOTPRINT_GRAD;
    g_NvidiaExt[index].numOutputsForIncCounter = 4;

    // result is returned as the return value of IncrementCounter on fake UAV slot
    uint4 op;
    op.x = g_NvidiaExt.IncrementCounter();
    op.y = g_NvidiaExt.IncrementCounter();
    op.z = g_NvidiaExt.IncrementCounter();
    op.w = g_NvidiaExt.IncrementCounter();
    return op;
}

// returns value of special register - specify subopcode from any of NV_SPECIALOP_* specified in nvShaderExtnEnums.h - other opcodes undefined behavior
uint __NvGetSpecial(uint subOpCode)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_GET_SPECIAL;
    g_NvidiaExt[index].src0u.x = subOpCode;
    return g_NvidiaExt.IncrementCounter();
}

// predicate is returned in laneValid indicating if srcLane is in range and val from specified lane is returned.
int __NvShflGeneric(int val, uint srcLane, uint maskClampVal, out uint laneValid)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].src0u.x  =  val;                             // variable to be shuffled
    g_NvidiaExt[index].src0u.y  =  srcLane;                         // source lane
    g_NvidiaExt[index].src0u.z  =  maskClampVal;
    g_NvidiaExt[index].opcode   =  NV_EXTN_OP_SHFL_GENERIC;
    g_NvidiaExt[index].numOutputsForIncCounter = 2;

    laneValid = asuint(g_NvidiaExt.IncrementCounter());
    return g_NvidiaExt.IncrementCounter();
}

//----------------------------------------------------------------------------//

// DXR RayQuery functions

#if __SHADER_TARGET_MAJOR > 6 || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 5)

uint __NvRtGetCandidateClusterID(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_GET_CANDIDATE_CLUSTER_ID;
    g_NvidiaExt[index].src0u.x = rqFlags;
    return g_NvidiaExt.IncrementCounter();
}

uint __NvRtGetCommittedClusterID(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_GET_COMMITTED_CLUSTER_ID;
    g_NvidiaExt[index].src0u.x = rqFlags;
    return g_NvidiaExt.IncrementCounter();
}

float3x3 __NvRtCandidateTriangleObjectPositions(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_CANDIDATE_TRIANGLE_OBJECT_POSITIONS;
    g_NvidiaExt[index].src0u.x = rqFlags;

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

float3x3 __NvRtCommittedTriangleObjectPositions(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_COMMITTED_TRIANGLE_OBJECT_POSITIONS;
    g_NvidiaExt[index].src0u.x = rqFlags;

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

bool __NvRtCandidateIsNonOpaqueSphere(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_CANDIDATE_IS_NONOPAQUE_SPHERE;
    g_NvidiaExt[index].src0u.x = rqFlags;
    uint ret = g_NvidiaExt.IncrementCounter();
    return ret != 0;
}

bool __NvRtCandidateIsNonOpaqueLss(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_CANDIDATE_IS_NONOPAQUE_LSS;
    g_NvidiaExt[index].src0u.x = rqFlags;
    uint ret = g_NvidiaExt.IncrementCounter();
    return ret != 0;
}

float __NvRtCandidateLssHitParameter(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_CANDIDATE_LSS_HIT_PARAMETER;
    g_NvidiaExt[index].src0u.x = rqFlags;
    float ret = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

float4 __NvRtCandidateSphereObjectPositionAndRadius(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_CANDIDATE_SPHERE_OBJECT_POSITION_AND_RADIUS;
    g_NvidiaExt[index].src0u.x = rqFlags;

    float4 ret;
    ret[0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[3] = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

float2x4 __NvRtCandidateLssObjectPositionsAndRadii(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_CANDIDATE_LSS_OBJECT_POSITIONS_AND_RADII;
    g_NvidiaExt[index].src0u.x = rqFlags;

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

float __NvRtCandidateBuiltinPrimitiveRayT(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_CANDIDATE_BUILTIN_PRIMITIVE_RAY_T;
    g_NvidiaExt[index].src0u.x = rqFlags;
    float ret = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

bool __NvRtCommittedIsSphere(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_COMMITTED_IS_SPHERE;
    g_NvidiaExt[index].src0u.x = rqFlags;
    uint ret = g_NvidiaExt.IncrementCounter();
    return ret != 0;
}

bool __NvRtCommittedIsLss(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_COMMITTED_IS_LSS;
    g_NvidiaExt[index].src0u.x = rqFlags;
    uint ret = g_NvidiaExt.IncrementCounter();
    return ret != 0;
}

float __NvRtCommittedLssHitParameter(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_COMMITTED_LSS_HIT_PARAMETER;
    g_NvidiaExt[index].src0u.x = rqFlags;
    float ret = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

float4 __NvRtCommittedSphereObjectPositionAndRadius(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_COMMITTED_SPHERE_OBJECT_POSITION_AND_RADIUS;
    g_NvidiaExt[index].src0u.x = rqFlags;

    float4 ret;
    ret[0] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[1] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[2] = asfloat(g_NvidiaExt.IncrementCounter());
    ret[3] = asfloat(g_NvidiaExt.IncrementCounter());
    return ret;
}

float2x4 __NvRtCommittedLssObjectPositionsAndRadii(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_COMMITTED_LSS_OBJECT_POSITIONS_AND_RADII;
    g_NvidiaExt[index].src0u.x = rqFlags;

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

void __NvRtCommitNonOpaqueBuiltinPrimitiveHit(uint rqFlags)
{
    uint index = g_NvidiaExt.IncrementCounter();
    g_NvidiaExt[index].opcode = NV_EXTN_OP_RT_COMMIT_NONOPAQUE_BUILTIN_PRIMITIVE_HIT;
    g_NvidiaExt[index].src0u.x = rqFlags;
    uint handle = g_NvidiaExt.IncrementCounter();
}

#endif
