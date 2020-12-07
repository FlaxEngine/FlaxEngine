//-------------------------------------------------------------------------------------
// DirectXMesh.inl
//  
// DirectX Mesh Geometry Library
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//-------------------------------------------------------------------------------------

#pragma once

//=====================================================================================
// DXGI Format Utilities
//=====================================================================================
_Use_decl_annotations_
inline bool __cdecl IsValidVB(DXGI_FORMAT fmt)
{
    return BytesPerElement(fmt) != 0;
}

_Use_decl_annotations_
inline bool __cdecl IsValidIB(DXGI_FORMAT fmt)
{
    return (fmt == DXGI_FORMAT_R32_UINT || fmt == DXGI_FORMAT_R16_UINT) != 0;
}
