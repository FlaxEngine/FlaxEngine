//-------------------------------------------------------------------------------------
// DirectXMesh.h
//  
// DirectX Mesh Geometry Library
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//-------------------------------------------------------------------------------------

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <stdint.h>

#if !defined(__d3d11_h__) && !defined(__d3d11_x_h__) && !defined(__d3d12_h__) && !defined(__d3d12_x_h__)
#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif
#endif

#include <directxmath.h>

#define DIRECTX_MESH_VERSION 130


namespace DirectX
{
    //---------------------------------------------------------------------------------
    // DXGI Format Utilities
    bool __cdecl IsValidVB(_In_ DXGI_FORMAT fmt);
    bool __cdecl IsValidIB(_In_ DXGI_FORMAT fmt);
    size_t __cdecl BytesPerElement(_In_ DXGI_FORMAT fmt);


    //---------------------------------------------------------------------------------
    // Input Layout Descriptor Utilities
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    bool __cdecl IsValid(_In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl);
    void __cdecl ComputeInputLayout(
        _In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl,
        _Out_writes_opt_(nDecl) uint32_t* offsets,
        _Out_writes_opt_(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) uint32_t* strides);
#endif

#if defined(__d3d12_h__) || defined(__d3d12_x_h__)
    bool __cdecl IsValid(const D3D12_INPUT_LAYOUT_DESC& vbDecl);
    void __cdecl ComputeInputLayout(
        const D3D12_INPUT_LAYOUT_DESC& vbDecl,
        _Out_writes_opt_(vbDecl.NumElements) uint32_t* offsets,
        _Out_writes_opt_(D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) uint32_t* strides);
#endif

    //---------------------------------------------------------------------------------
    // Attribute Utilities
    std::vector<std::pair<size_t, size_t>> __cdecl ComputeSubsets(_In_reads_opt_(nFaces) const uint32_t* attributes, _In_ size_t nFaces);
        // Returns a list of face offset,counts for attribute groups

    //---------------------------------------------------------------------------------
    // Mesh Optimization Utilities
    void __cdecl ComputeVertexCacheMissRate(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces, _In_ size_t nVerts,
        _In_ size_t cacheSize, _Out_ float& acmr, _Out_ float& atvr);
    void __cdecl ComputeVertexCacheMissRate(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces, _In_ size_t nVerts,
        _In_ size_t cacheSize, _Out_ float& acmr, _Out_ float& atvr);
        // Compute the average cache miss ratio and average triangle vertex reuse for the post-transform vertex cache

    //---------------------------------------------------------------------------------
    // Vertex Buffer Reader/Writer

    class VBReader
    {
    public:
        VBReader() noexcept(false);
        VBReader(VBReader&& moveFrom) noexcept;
        VBReader& operator= (VBReader&& moveFrom) noexcept;

        VBReader(VBReader const&) = delete;
        VBReader& operator= (VBReader const&) = delete;

        ~VBReader();

    #if defined(__d3d11_h__) || defined(__d3d11_x_h__)
        HRESULT __cdecl Initialize(_In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl);
            // Does not support VB decls with D3D11_INPUT_PER_INSTANCE_DATA
    #endif

    #if defined(__d3d12_h__) || defined(__d3d12_x_h__)
        HRESULT __cdecl Initialize(const D3D12_INPUT_LAYOUT_DESC& vbDecl);
            // Does not support VB decls with D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
    #endif

        HRESULT __cdecl AddStream(_In_reads_bytes_(stride*nVerts) const void* vb, _In_ size_t nVerts, _In_ size_t inputSlot, _In_ size_t stride = 0);
            // Add vertex buffer to reader

        HRESULT __cdecl Read(_Out_writes_(count) XMVECTOR* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
            // Extracts data elements from vertex buffer

        HRESULT __cdecl Read(_Out_writes_(count) float* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Read(_Out_writes_(count) XMFLOAT2* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Read(_Out_writes_(count) XMFLOAT3* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Read(_Out_writes_(count) XMFLOAT4* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
            // Helpers for data extraction

        void __cdecl Release();

    #if defined(__d3d11_h__) || defined(__d3d11_x_h__)
        const D3D11_INPUT_ELEMENT_DESC* GetElement(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const
        {
            return GetElement11(semanticName, semanticIndex);
        }

        const D3D11_INPUT_ELEMENT_DESC* __cdecl GetElement11(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const;
    #endif

    #if defined(__d3d12_h__) || defined(__d3d12_x_h__)
        const D3D12_INPUT_ELEMENT_DESC* __cdecl GetElement12(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const;
    #endif

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };

    class VBWriter
    {
    public:
        VBWriter() noexcept(false);
        VBWriter(VBWriter&& moveFrom) noexcept;
        VBWriter& operator= (VBWriter&& moveFrom) noexcept;

        VBWriter(VBWriter const&) = delete;
        VBWriter& operator= (VBWriter const&) = delete;

        ~VBWriter();

    #if defined(__d3d11_h__) || defined(__d3d11_x_h__)
        HRESULT __cdecl Initialize(_In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl);
            // Does not support VB decls with D3D11_INPUT_PER_INSTANCE_DATA
    #endif

    #if defined(__d3d12_h__) || defined(__d3d12_x_h__)
        HRESULT __cdecl Initialize(const D3D12_INPUT_LAYOUT_DESC& vbDecl);
            // Does not support VB decls with D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
    #endif

        HRESULT __cdecl AddStream(_Out_writes_bytes_(stride*nVerts) void* vb, _In_ size_t nVerts, _In_ size_t inputSlot, _In_ size_t stride = 0);
            // Add vertex buffer to writer

        HRESULT __cdecl Write(_In_reads_(count) const XMVECTOR* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
            // Inserts data elements into vertex buffer

        HRESULT __cdecl Write(_In_reads_(count) const float* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Write(_In_reads_(count) const XMFLOAT2* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Write(_In_reads_(count) const XMFLOAT3* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Write(_In_reads_(count) const XMFLOAT4* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
            // Helpers for data insertion

        void __cdecl Release();

    #if defined(__d3d11_h__) || defined(__d3d11_x_h__)
        const D3D11_INPUT_ELEMENT_DESC* __cdecl GetElement(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const
        {
            return GetElement11(semanticName, semanticIndex);
        }

        const D3D11_INPUT_ELEMENT_DESC* __cdecl GetElement11(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const;
    #endif

    #if defined(__d3d12_h__) || defined(__d3d12_x_h__)
        const D3D12_INPUT_ELEMENT_DESC* __cdecl GetElement12(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const;
    #endif

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };

    //---------------------------------------------------------------------------------
    // Adjacency Computation

    HRESULT __cdecl GenerateAdjacencyAndPointReps(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_ float epsilon,
        _Out_writes_opt_(nVerts) uint32_t* pointRep,
        _Out_writes_opt_(nFaces * 3) uint32_t* adjacency);
    HRESULT __cdecl GenerateAdjacencyAndPointReps(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_ float epsilon,
        _Out_writes_opt_(nVerts) uint32_t* pointRep,
        _Out_writes_opt_(nFaces * 3) uint32_t* adjacency);
        // If pointRep is null, it still generates them internally as they are needed for the final adjacency computation

    HRESULT __cdecl ConvertPointRepsToAdjacency(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_reads_opt_(nVerts) const uint32_t* pointRep,
        _Out_writes_(nFaces * 3) uint32_t* adjacency);
    HRESULT __cdecl ConvertPointRepsToAdjacency(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_reads_opt_(nVerts) const uint32_t* pointRep,
        _Out_writes_(nFaces * 3) uint32_t* adjacency);
        // If pointRep is null, assumes an identity

    HRESULT __cdecl GenerateGSAdjacency(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* pointRep,
        _In_reads_(nFaces * 3) const uint32_t* adjacency, _In_ size_t nVerts,
        _Out_writes_(nFaces * 6) uint16_t* indicesAdj);
    HRESULT __cdecl GenerateGSAdjacency(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* pointRep,
        _In_reads_(nFaces * 3) const uint32_t* adjacency, _In_ size_t nVerts,
        _Out_writes_(nFaces * 6) uint32_t* indicesAdj);
        // Generates an IB suitable for Geometry Shader using D3D1x_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ

    //---------------------------------------------------------------------------------
    // Normals, Tangents, and Bi-Tangents Computation

    enum CNORM_FLAGS
    {
        CNORM_DEFAULT                   = 0x0,
            // Default is to compute normals using weight-by-angle

        CNORM_WEIGHT_BY_AREA            = 0x1,
            // Computes normals using weight-by-area

        CNORM_WEIGHT_EQUAL              = 0x2,
            // Compute normals with equal weights

        CNORM_WIND_CW                   = 0x4,
            // Vertices are clock-wise (defaults to CCW)
    };

    HRESULT __cdecl ComputeNormals(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_ DWORD flags,
        _Out_writes_(nVerts) XMFLOAT3* normals);
    HRESULT __cdecl ComputeNormals(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_ DWORD flags,
        _Out_writes_(nVerts) XMFLOAT3* normals);
        // Computes vertex normals

    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_opt_(nVerts) XMFLOAT3* tangents,
        _Out_writes_opt_(nVerts) XMFLOAT3* bitangents);
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_opt_(nVerts) XMFLOAT3* tangents,
        _Out_writes_opt_(nVerts) XMFLOAT3* bitangents);
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_opt_(nVerts) XMFLOAT4* tangents,
        _Out_writes_opt_(nVerts) XMFLOAT3* bitangents);
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_opt_(nVerts) XMFLOAT4* tangents,
        _Out_writes_opt_(nVerts) XMFLOAT3* bitangents);
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_(nVerts) XMFLOAT4* tangents);
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_(nVerts) XMFLOAT4* tangents);
        // Computes tangents and/or bi-tangents (optionally with handedness stored in .w)

    //---------------------------------------------------------------------------------
    // Mesh clean-up and validation

    enum VALIDATE_FLAGS
    {
        VALIDATE_DEFAULT                = 0x0,

        VALIDATE_BACKFACING             = 0x1,
            // Check for duplicate neighbor from triangle (requires adjacency)

        VALIDATE_BOWTIES                = 0x2,
            // Check for two fans of triangles using the same vertex (requires adjacency)

        VALIDATE_DEGENERATE             = 0x4,
            // Check for degenerate triangles

        VALIDATE_UNUSED                 = 0x8,
            // Check for issues with 'unused' triangles

        VALIDATE_ASYMMETRIC_ADJ         = 0x10,
            // Checks that neighbors are symmetric (requires adjacency)
    };

    HRESULT __cdecl Validate(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_ size_t nVerts, _In_reads_opt_(nFaces * 3) const uint32_t* adjacency,
        _In_ DWORD flags, _In_opt_ std::wstring* msgs = nullptr);
    HRESULT __cdecl Validate(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_ size_t nVerts, _In_reads_opt_(nFaces * 3) const uint32_t* adjacency,
        _In_ DWORD flags, _In_opt_ std::wstring* msgs = nullptr);
        // Checks the mesh for common problems, return 'S_OK' if no problems were found

    HRESULT __cdecl Clean(
        _Inout_updates_all_(nFaces * 3) uint16_t* indices, _In_ size_t nFaces,
        _In_ size_t nVerts, _Inout_updates_all_opt_(nFaces * 3) uint32_t* adjacency,
        _In_reads_opt_(nFaces) const uint32_t* attributes,
        _Inout_ std::vector<uint32_t>& dupVerts, _In_ bool breakBowties = false);
    HRESULT __cdecl Clean(
        _Inout_updates_all_(nFaces * 3) uint32_t* indices, _In_ size_t nFaces,
        _In_ size_t nVerts, _Inout_updates_all_opt_(nFaces * 3) uint32_t* adjacency,
        _In_reads_opt_(nFaces) const uint32_t* attributes,
        _Inout_ std::vector<uint32_t>& dupVerts, _In_ bool breakBowties = false);
        // Cleans the mesh, splitting vertices if needed

    //---------------------------------------------------------------------------------
    // Mesh utilities

    HRESULT __cdecl WeldVertices(
        _Inout_updates_all_(nFaces * 3) uint16_t* indices, _In_ size_t nFaces,
        _In_ size_t nVerts, _In_reads_(nVerts) const uint32_t* pointRep,
        _Out_writes_opt_(nVerts) uint32_t* vertexRemap,
        _In_ std::function<bool __cdecl(uint32_t v0, uint32_t v1)> weldTest);
    HRESULT __cdecl WeldVertices(
        _Inout_updates_all_(nFaces * 3) uint32_t* indices, _In_ size_t nFaces,
        _In_ size_t nVerts, _In_reads_(nVerts) const uint32_t* pointRep,
        _Out_writes_opt_(nVerts) uint32_t* vertexRemap,
        _In_ std::function<bool __cdecl(uint32_t v0, uint32_t v1)> weldTest);
        // Welds vertices together based on a test function

    //---------------------------------------------------------------------------------
    // Mesh Optimization

    HRESULT __cdecl AttributeSort(
        _In_ size_t nFaces, _Inout_updates_all_(nFaces) uint32_t* attributes,
        _Out_writes_(nFaces) uint32_t* faceRemap);
        // Reorders faces by attribute id

    enum OPTFACES
    {
        OPTFACES_V_DEFAULT      = 12,
        OPTFACES_R_DEFAULT      = 7,
            // Default vertex cache size and restart threshold which is considered 'device independent'

        OPTFACES_LRU_DEFAULT    = 32,
            // Default vertex cache size for the LRU algorithm

        OPTFACES_V_STRIPORDER   = 0,
            // Indicates no vertex cache optimization, only reordering into strips
    };

    HRESULT __cdecl OptimizeFaces(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nFaces * 3) const uint32_t* adjacency,
        _Out_writes_(nFaces) uint32_t* faceRemap,
        _In_ uint32_t vertexCache = OPTFACES_V_DEFAULT,
        _In_ uint32_t restart = OPTFACES_R_DEFAULT);
    HRESULT __cdecl OptimizeFaces(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nFaces * 3) const uint32_t* adjacency,
        _Out_writes_(nFaces) uint32_t* faceRemap,
        _In_ uint32_t vertexCache = OPTFACES_V_DEFAULT,
        _In_ uint32_t restart = OPTFACES_R_DEFAULT);
    HRESULT __cdecl OptimizeFacesLRU(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _Out_writes_(nFaces) uint32_t* faceRemap,
        _In_ uint32_t lruCacheSize = OPTFACES_LRU_DEFAULT);
    HRESULT __cdecl OptimizeFacesLRU(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _Out_writes_(nFaces) uint32_t* faceRemap,
        _In_ uint32_t lruCacheSize = OPTFACES_LRU_DEFAULT);
        // Reorders faces to increase hit rate of vertex caches

    HRESULT __cdecl OptimizeFacesEx(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nFaces * 3) const uint32_t* adjacency,
        _In_reads_(nFaces) const uint32_t* attributes,
        _Out_writes_(nFaces) uint32_t* faceRemap,
        _In_ uint32_t vertexCache = OPTFACES_V_DEFAULT,
        _In_ uint32_t restart = OPTFACES_R_DEFAULT);
    HRESULT __cdecl OptimizeFacesEx(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nFaces * 3) const uint32_t* adjacency,
        _In_reads_(nFaces) const uint32_t* attributes,
        _Out_writes_(nFaces) uint32_t* faceRemap,
        _In_ uint32_t vertexCache = OPTFACES_V_DEFAULT,
        _In_ uint32_t restart = OPTFACES_R_DEFAULT);
    HRESULT __cdecl OptimizeFacesLRUEx(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* attributes,
        _Out_writes_(nFaces) uint32_t* faceRemap,
        _In_ uint32_t lruCacheSize = OPTFACES_LRU_DEFAULT);
    HRESULT __cdecl OptimizeFacesLRUEx(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* attributes,
        _Out_writes_(nFaces) uint32_t* faceRemap,
        _In_ uint32_t lruCacheSize = OPTFACES_LRU_DEFAULT);
        // Attribute group version of OptimizeFaces

    HRESULT __cdecl OptimizeVertices(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces, _In_ size_t nVerts,
        _Out_writes_(nVerts) uint32_t* vertexRemap, _Out_opt_ size_t* trailingUnused = nullptr);
    HRESULT __cdecl OptimizeVertices(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces, _In_ size_t nVerts,
        _Out_writes_(nVerts) uint32_t* vertexRemap, _Out_opt_ size_t* trailingUnused = nullptr);
        // Reorders vertices in order of use

    //---------------------------------------------------------------------------------
    // Remap functions

    HRESULT __cdecl ReorderIB(
        _In_reads_(nFaces * 3) const uint16_t* ibin, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* faceRemap,
        _Out_writes_(nFaces * 3) uint16_t* ibout);
    HRESULT __cdecl ReorderIB(
        _Inout_updates_all_(nFaces * 3) uint16_t* ib, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* faceRemap);
    HRESULT __cdecl ReorderIB(
        _In_reads_(nFaces * 3) const uint32_t* ibin, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* faceRemap,
        _Out_writes_(nFaces * 3) uint32_t* ibout);
    HRESULT __cdecl ReorderIB(
        _Inout_updates_all_(nFaces * 3) uint32_t* ib, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* faceRemap);
        // Applies a face remap reordering to an index buffer

    HRESULT __cdecl ReorderIBAndAdjacency(
        _In_reads_(nFaces * 3) const uint16_t* ibin, _In_ size_t nFaces, _In_reads_(nFaces * 3) const uint32_t* adjin,
        _In_reads_(nFaces) const uint32_t* faceRemap,
        _Out_writes_(nFaces * 3) uint16_t* ibout, _Out_writes_(nFaces * 3) uint32_t* adjout);
    HRESULT __cdecl ReorderIBAndAdjacency(
        _Inout_updates_all_(nFaces * 3) uint16_t* ib, _In_ size_t nFaces, _Inout_updates_all_(nFaces * 3) uint32_t* adj,
        _In_reads_(nFaces) const uint32_t* faceRemap);
    HRESULT __cdecl ReorderIBAndAdjacency(
        _In_reads_(nFaces * 3) const uint32_t* ibin, _In_ size_t nFaces, _In_reads_(nFaces * 3) const uint32_t* adjin,
        _In_reads_(nFaces) const uint32_t* faceRemap,
        _Out_writes_(nFaces * 3) uint32_t* ibout, _Out_writes_(nFaces * 3) uint32_t* adjout);
    HRESULT __cdecl ReorderIBAndAdjacency(
        _Inout_updates_all_(nFaces * 3) uint32_t* ib, _In_ size_t nFaces, _Inout_updates_all_(nFaces * 3) uint32_t* adj,
        _In_reads_(nFaces) const uint32_t* faceRemap);
        // Applies a face remap reordering to an index buffer and adjacency

    HRESULT __cdecl FinalizeIB(
        _In_reads_(nFaces * 3) const uint16_t* ibin, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts,
        _Out_writes_(nFaces * 3) uint16_t* ibout);
    HRESULT __cdecl FinalizeIB(
        _Inout_updates_all_(nFaces * 3) uint16_t* ib, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts);
    HRESULT __cdecl FinalizeIB(
        _In_reads_(nFaces * 3) const uint32_t* ibin, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts,
        _Out_writes_(nFaces * 3) uint32_t* ibout);
    HRESULT __cdecl FinalizeIB(
        _Inout_updates_all_(nFaces * 3) uint32_t* ib, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts);
        // Applies a vertex remap reordering to an index buffer

    HRESULT __cdecl FinalizeVB(
        _In_reads_bytes_(nVerts*stride) const void* vbin, _In_ size_t stride, _In_ size_t nVerts,
        _In_reads_opt_(nDupVerts) const uint32_t* dupVerts, _In_ size_t nDupVerts,
        _In_reads_opt_(nVerts + nDupVerts) const uint32_t* vertexRemap,
        _Out_writes_bytes_((nVerts + nDupVerts)*stride) void* vbout);
    HRESULT __cdecl FinalizeVB(
        _Inout_updates_bytes_all_(nVerts*stride) void* vb, _In_ size_t stride, _In_ size_t nVerts,
        _In_reads_(nVerts) const uint32_t* vertexRemap);
        // Applies a vertex remap and/or a vertex duplication set to a vertex buffer

    HRESULT __cdecl FinalizeVBAndPointReps(
        _In_reads_bytes_(nVerts*stride) const void* vbin, _In_ size_t stride, _In_ size_t nVerts,
        _In_reads_(nVerts) const uint32_t* prin,
        _In_reads_opt_(nDupVerts) const uint32_t* dupVerts, _In_ size_t nDupVerts,
        _In_reads_opt_(nVerts + nDupVerts) const uint32_t* vertexRemap,
        _Out_writes_bytes_((nVerts + nDupVerts)*stride) void* vbout,
        _Out_writes_(nVerts + nDupVerts) uint32_t* prout);
    HRESULT __cdecl FinalizeVBAndPointReps(
        _Inout_updates_bytes_all_(nVerts*stride) void* vb, _In_ size_t stride, _In_ size_t nVerts,
        _Inout_updates_all_(nVerts) uint32_t* pointRep,
        _In_reads_(nVerts) const uint32_t* vertexRemap);
        // Applies a vertex remap and/or a vertex duplication set to a vertex buffer and point representatives

    HRESULT __cdecl CompactVB(
        _In_reads_bytes_(nVerts*stride) const void* vbin, _In_ size_t stride, _In_ size_t nVerts,
        _In_ size_t trailingUnused,
        _In_reads_opt_(nVerts) const uint32_t* vertexRemap,
        _Out_writes_bytes_((nVerts - trailingUnused)*stride) void* vbout);
        // Applies a vertex remap which contains a known number of unused entries at the end

#include "DirectXMesh.inl"

} // namespace
