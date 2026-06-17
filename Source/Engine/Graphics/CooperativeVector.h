// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

class GPUBuffer;

// Cooperative vectors (NVIDIA Neural Shading) public graphics API.
//
// Cooperative-vector matrix-vector ops (dx::linalg MatVecMulAdd / OuterProductAccumulate) read
// matrices stored in hardware "optimal" layouts. Weights are authored row-major and converted to
// the optimal layout on the GPU via GPUContext::ConvertCooperativeVectorMatrices. The required
// destination byte size for a given layout/type is queried with GPUDevice::GetCooperativeVectorMatrixSize.
//
// Only available when GPUDevice::Limits.HasCooperativeVector is set (D3D12 with the Agility SDK
// preview + a supporting driver). Values mirror the backend enums (D3D12 D3D12_LINEAR_ALGEBRA_*).

/// <summary>
/// Element data type of a cooperative-vector matrix or vector stored in memory.
/// </summary>
enum class CooperativeVectorDataType
{
    SInt16 = 2,
    UInt16 = 3,
    SInt32 = 4,
    UInt32 = 5,
    Float16 = 7,
    Float32 = 8,
    SInt8x4Packed = 16,
    UInt8x4Packed = 17,
    UInt8 = 18,
    SInt8 = 19,
    FloatE4M3 = 20,
    FloatE5M2 = 21,
};

/// <summary>
/// Physical layout of a cooperative-vector matrix in memory.
/// </summary>
enum class CooperativeVectorMatrixLayout
{
    // Row-major: elements stored row by row, Stride bytes between rows.
    RowMajor = 0,
    // Column-major: elements stored column by column, Stride bytes between columns.
    ColumnMajor = 1,
    // Hardware-optimal layout for matrix-vector multiply (inference). Stride must be 0.
    MulOptimal = 2,
    // Hardware-optimal layout for outer-product accumulation (training gradients). Stride must be 0.
    OuterProductOptimal = 3,
};

/// <summary>
/// Describes a single GPU cooperative-vector matrix layout/precision conversion.
/// </summary>
struct CooperativeVectorMatrixConvert
{
    // Source matrix.
    GPUBuffer* SrcBuffer = nullptr;
    uint32 SrcOffset = 0;
    uint32 SrcSize = 0;
    CooperativeVectorDataType SrcDataType = CooperativeVectorDataType::Float32;
    CooperativeVectorMatrixLayout SrcLayout = CooperativeVectorMatrixLayout::RowMajor;
    uint32 SrcStride = 0; // bytes between rows/columns for RowMajor/ColumnMajor, 0 for optimal layouts

    // Destination matrix (DstSize must come from GPUDevice::GetCooperativeVectorMatrixSize).
    GPUBuffer* DstBuffer = nullptr;
    uint32 DstOffset = 0;
    uint32 DstSize = 0;
    CooperativeVectorDataType DstDataType = CooperativeVectorDataType::Float16;
    CooperativeVectorMatrixLayout DstLayout = CooperativeVectorMatrixLayout::MulOptimal;
    uint32 DstStride = 0;

    // Matrix dimensions (rows = outputs, columns = inputs).
    uint32 NumRows = 0;
    uint32 NumColumns = 0;
};
