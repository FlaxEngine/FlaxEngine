//-------------------------------------------------------------------------------------
// DirectXMesh.h
//
// DirectX Mesh Geometry Library
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//-------------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#if !defined(__d3d11_h__) && !defined(__d3d11_x_h__) && !defined(__d3d12_h__) && !defined(__d3d12_x_h__) && !defined(__XBOX_D3D12_X__)
#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#elif defined(_GAMING_XBOX)
#include <d3d12_x.h>
#elif defined(_XBOX_ONE) && defined(_TITLE)
#error This library no longer supports legacy Xbox One XDK
#else
#include <d3d11_1.h>
#endif
#endif
#else // !WIN32
#include <directx/dxgiformat.h>
#include <wsl/winadapter.h>
#endif

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <DirectXPackedVector.h>

#define DIRECTX_MESH_VERSION 166


namespace DirectX
{
    //---------------------------------------------------------------------------------
    // DXGI Format Utilities
    bool __cdecl IsValidVB(_In_ DXGI_FORMAT fmt) noexcept;
    constexpr bool __cdecl IsValidIB(_In_ DXGI_FORMAT fmt) noexcept;
    size_t __cdecl BytesPerElement(_In_ DXGI_FORMAT fmt) noexcept;


    //---------------------------------------------------------------------------------
    // Input Layout Descriptor Utilities
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    bool __cdecl IsValid(_In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl) noexcept;
    void __cdecl ComputeInputLayout(
        _In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl,
        _Out_writes_opt_(nDecl) uint32_t* offsets,
        _Out_writes_opt_(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) uint32_t* strides) noexcept;
#endif

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
    bool __cdecl IsValid(const D3D12_INPUT_LAYOUT_DESC& vbDecl) noexcept;
    void __cdecl ComputeInputLayout(
        const D3D12_INPUT_LAYOUT_DESC& vbDecl,
        _Out_writes_opt_(vbDecl.NumElements) uint32_t* offsets,
        _Out_writes_opt_(D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT) uint32_t* strides) noexcept;
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
        VBReader(VBReader&&) noexcept;
        VBReader& operator= (VBReader&&) noexcept;

        VBReader(VBReader const&) = delete;
        VBReader& operator= (VBReader const&) = delete;

        ~VBReader();

    #if defined(__d3d11_h__) || defined(__d3d11_x_h__)
        HRESULT __cdecl Initialize(_In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl);
            // Does not support VB decls with D3D11_INPUT_PER_INSTANCE_DATA
    #endif

    #if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
        HRESULT __cdecl Initialize(const D3D12_INPUT_LAYOUT_DESC& vbDecl);
            // Does not support VB decls with D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
    #endif

        HRESULT __cdecl AddStream(_In_reads_bytes_(stride*nVerts) const void* vb, _In_ size_t nVerts, _In_ size_t inputSlot, _In_ size_t stride = 0) noexcept;
            // Add vertex buffer to reader

        HRESULT __cdecl Read(_Out_writes_(count) XMVECTOR* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
            // Extracts data elements from vertex buffer

        HRESULT __cdecl Read(_Out_writes_(count) float* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Read(_Out_writes_(count) XMFLOAT2* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Read(_Out_writes_(count) XMFLOAT3* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Read(_Out_writes_(count) XMFLOAT4* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
            // Helpers for data extraction

        void __cdecl Release() noexcept;

    #if defined(__d3d11_h__) || defined(__d3d11_x_h__)
        const D3D11_INPUT_ELEMENT_DESC* GetElement(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const
        {
            return GetElement11(semanticName, semanticIndex);
        }

        const D3D11_INPUT_ELEMENT_DESC* __cdecl GetElement11(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const;
    #endif

    #if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
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
        VBWriter(VBWriter&&) noexcept;
        VBWriter& operator= (VBWriter&&) noexcept;

        VBWriter(VBWriter const&) = delete;
        VBWriter& operator= (VBWriter const&) = delete;

        ~VBWriter();

    #if defined(__d3d11_h__) || defined(__d3d11_x_h__)
        HRESULT __cdecl Initialize(_In_reads_(nDecl) const D3D11_INPUT_ELEMENT_DESC* vbDecl, _In_ size_t nDecl);
            // Does not support VB decls with D3D11_INPUT_PER_INSTANCE_DATA
    #endif

    #if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
        HRESULT __cdecl Initialize(const D3D12_INPUT_LAYOUT_DESC& vbDecl);
            // Does not support VB decls with D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
    #endif

        HRESULT __cdecl AddStream(_Out_writes_bytes_(stride*nVerts) void* vb, _In_ size_t nVerts, _In_ size_t inputSlot, _In_ size_t stride = 0) noexcept;
            // Add vertex buffer to writer

        HRESULT __cdecl Write(_In_reads_(count) const XMVECTOR* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
            // Inserts data elements into vertex buffer

        HRESULT __cdecl Write(_In_reads_(count) const float* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Write(_In_reads_(count) const XMFLOAT2* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Write(_In_reads_(count) const XMFLOAT3* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
        HRESULT __cdecl Write(_In_reads_(count) const XMFLOAT4* buffer, _In_z_ const char* semanticName, _In_ unsigned int semanticIndex, _In_ size_t count, bool x2bias = false) const;
            // Helpers for data insertion

        void __cdecl Release() noexcept;

    #if defined(__d3d11_h__) || defined(__d3d11_x_h__)
        const D3D11_INPUT_ELEMENT_DESC* __cdecl GetElement(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const
        {
            return GetElement11(semanticName, semanticIndex);
        }

        const D3D11_INPUT_ELEMENT_DESC* __cdecl GetElement11(_In_z_ const char* semanticName, _In_ unsigned int semanticIndex) const;
    #endif

    #if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
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
        _Out_writes_(nFaces * 6) uint16_t* indicesAdj) noexcept;
    HRESULT __cdecl GenerateGSAdjacency(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* pointRep,
        _In_reads_(nFaces * 3) const uint32_t* adjacency, _In_ size_t nVerts,
        _Out_writes_(nFaces * 6) uint32_t* indicesAdj) noexcept;
        // Generates an IB suitable for Geometry Shader using D3D1x_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ

    //---------------------------------------------------------------------------------
    // Normals, Tangents, and Bi-Tangents Computation

    enum CNORM_FLAGS : unsigned long
    {
        CNORM_DEFAULT = 0x0,
        // Default is to compute normals using weight-by-angle

        CNORM_WEIGHT_BY_AREA = 0x1,
        // Computes normals using weight-by-area

        CNORM_WEIGHT_EQUAL = 0x2,
        // Compute normals with equal weights

        CNORM_WIND_CW = 0x4,
        // Vertices are clock-wise (defaults to CCW)
    };

    HRESULT __cdecl ComputeNormals(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_ CNORM_FLAGS flags,
        _Out_writes_(nVerts) XMFLOAT3* normals) noexcept;
    HRESULT __cdecl ComputeNormals(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_ CNORM_FLAGS flags,
        _Out_writes_(nVerts) XMFLOAT3* normals) noexcept;
        // Computes vertex normals

    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_opt_(nVerts) XMFLOAT3* tangents,
        _Out_writes_opt_(nVerts) XMFLOAT3* bitangents) noexcept;
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_opt_(nVerts) XMFLOAT3* tangents,
        _Out_writes_opt_(nVerts) XMFLOAT3* bitangents) noexcept;
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_opt_(nVerts) XMFLOAT4* tangents,
        _Out_writes_opt_(nVerts) XMFLOAT3* bitangents) noexcept;
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_opt_(nVerts) XMFLOAT4* tangents,
        _Out_writes_opt_(nVerts) XMFLOAT3* bitangents) noexcept;
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_(nVerts) XMFLOAT4* tangents) noexcept;
    HRESULT __cdecl ComputeTangentFrame(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions,
        _In_reads_(nVerts) const XMFLOAT3* normals,
        _In_reads_(nVerts) const XMFLOAT2* texcoords, _In_ size_t nVerts,
        _Out_writes_(nVerts) XMFLOAT4* tangents) noexcept;
        // Computes tangents and/or bi-tangents (optionally with handedness stored in .w)

    //---------------------------------------------------------------------------------
    // Mesh clean-up and validation

    enum VALIDATE_FLAGS : unsigned long
    {
        VALIDATE_DEFAULT = 0x0,

        VALIDATE_BACKFACING = 0x1,
        // Check for duplicate neighbor from triangle (requires adjacency)

        VALIDATE_BOWTIES = 0x2,
        // Check for two fans of triangles using the same vertex (requires adjacency)

        VALIDATE_DEGENERATE = 0x4,
        // Check for degenerate triangles

        VALIDATE_UNUSED = 0x8,
        // Check for issues with 'unused' triangles

        VALIDATE_ASYMMETRIC_ADJ = 0x10,
        // Checks that neighbors are symmetric (requires adjacency)
    };

    HRESULT __cdecl Validate(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_ size_t nVerts, _In_reads_opt_(nFaces * 3) const uint32_t* adjacency,
        _In_ VALIDATE_FLAGS flags, _In_opt_ std::wstring* msgs = nullptr);
    HRESULT __cdecl Validate(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_ size_t nVerts, _In_reads_opt_(nFaces * 3) const uint32_t* adjacency,
        _In_ VALIDATE_FLAGS flags, _In_opt_ std::wstring* msgs = nullptr);
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

    HRESULT __cdecl ConcatenateMesh(
        _In_ size_t nFaces,
        _In_ size_t nVerts,
        _Out_writes_(nFaces) uint32_t* faceDestMap,
        _Out_writes_(nVerts) uint32_t* vertexDestMap,
        _Inout_ size_t& totalFaces,
        _Inout_ size_t& totalVerts) noexcept;
        // Merge meshes together

    //---------------------------------------------------------------------------------
    // Mesh Optimization

    HRESULT __cdecl AttributeSort(
        _In_ size_t nFaces, _Inout_updates_all_(nFaces) uint32_t* attributes,
        _Out_writes_(nFaces) uint32_t* faceRemap);
        // Reorders faces by attribute id

    enum OPTFACES : uint32_t
    {
        OPTFACES_V_DEFAULT = 12,
        OPTFACES_R_DEFAULT = 7,
        // Default vertex cache size and restart threshold which is considered 'device independent'

        OPTFACES_LRU_DEFAULT = 32,
        // Default vertex cache size for the LRU algorithm

        OPTFACES_V_STRIPORDER = 0,
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
        _Out_writes_(nVerts) uint32_t* vertexRemap, _Out_opt_ size_t* trailingUnused = nullptr) noexcept;
    HRESULT __cdecl OptimizeVertices(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces, _In_ size_t nVerts,
        _Out_writes_(nVerts) uint32_t* vertexRemap, _Out_opt_ size_t* trailingUnused = nullptr) noexcept;
        // Reorders vertices in order of use

    //---------------------------------------------------------------------------------
    // Remap functions

    HRESULT __cdecl ReorderIB(
        _In_reads_(nFaces * 3) const uint16_t* ibin, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* faceRemap,
        _Out_writes_(nFaces * 3) uint16_t* ibout) noexcept;
    HRESULT __cdecl ReorderIB(
        _Inout_updates_all_(nFaces * 3) uint16_t* ib, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* faceRemap) noexcept;
    HRESULT __cdecl ReorderIB(
        _In_reads_(nFaces * 3) const uint32_t* ibin, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* faceRemap,
        _Out_writes_(nFaces * 3) uint32_t* ibout) noexcept;
    HRESULT __cdecl ReorderIB(
        _Inout_updates_all_(nFaces * 3) uint32_t* ib, _In_ size_t nFaces,
        _In_reads_(nFaces) const uint32_t* faceRemap) noexcept;
        // Applies a face remap reordering to an index buffer

    HRESULT __cdecl ReorderIBAndAdjacency(
        _In_reads_(nFaces * 3) const uint16_t* ibin, _In_ size_t nFaces, _In_reads_(nFaces * 3) const uint32_t* adjin,
        _In_reads_(nFaces) const uint32_t* faceRemap,
        _Out_writes_(nFaces * 3) uint16_t* ibout, _Out_writes_(nFaces * 3) uint32_t* adjout) noexcept;
    HRESULT __cdecl ReorderIBAndAdjacency(
        _Inout_updates_all_(nFaces * 3) uint16_t* ib, _In_ size_t nFaces, _Inout_updates_all_(nFaces * 3) uint32_t* adj,
        _In_reads_(nFaces) const uint32_t* faceRemap) noexcept;
    HRESULT __cdecl ReorderIBAndAdjacency(
        _In_reads_(nFaces * 3) const uint32_t* ibin, _In_ size_t nFaces, _In_reads_(nFaces * 3) const uint32_t* adjin,
        _In_reads_(nFaces) const uint32_t* faceRemap,
        _Out_writes_(nFaces * 3) uint32_t* ibout, _Out_writes_(nFaces * 3) uint32_t* adjout) noexcept;
    HRESULT __cdecl ReorderIBAndAdjacency(
        _Inout_updates_all_(nFaces * 3) uint32_t* ib, _In_ size_t nFaces, _Inout_updates_all_(nFaces * 3) uint32_t* adj,
        _In_reads_(nFaces) const uint32_t* faceRemap) noexcept;
        // Applies a face remap reordering to an index buffer and adjacency

    HRESULT __cdecl FinalizeIB(
        _In_reads_(nFaces * 3) const uint16_t* ibin, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts,
        _Out_writes_(nFaces * 3) uint16_t* ibout) noexcept;
    HRESULT __cdecl FinalizeIB(
        _Inout_updates_all_(nFaces * 3) uint16_t* ib, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts) noexcept;
    HRESULT __cdecl FinalizeIB(
        _In_reads_(nFaces * 3) const uint32_t* ibin, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts,
        _Out_writes_(nFaces * 3) uint32_t* ibout) noexcept;
    HRESULT __cdecl FinalizeIB(
        _Inout_updates_all_(nFaces * 3) uint32_t* ib, _In_ size_t nFaces,
        _In_reads_(nVerts) const uint32_t* vertexRemap, _In_ size_t nVerts) noexcept;
        // Applies a vertex remap reordering to an index buffer

    HRESULT __cdecl FinalizeVB(
        _In_reads_bytes_(nVerts*stride) const void* vbin, _In_ size_t stride, _In_ size_t nVerts,
        _In_reads_opt_(nDupVerts) const uint32_t* dupVerts, _In_ size_t nDupVerts,
        _In_reads_opt_(nVerts + nDupVerts) const uint32_t* vertexRemap,
        _Out_writes_bytes_((nVerts + nDupVerts)*stride) void* vbout) noexcept;
    HRESULT __cdecl FinalizeVB(
        _Inout_updates_bytes_all_(nVerts*stride) void* vb, _In_ size_t stride, _In_ size_t nVerts,
        _In_reads_(nVerts) const uint32_t* vertexRemap) noexcept;
        // Applies a vertex remap and/or a vertex duplication set to a vertex buffer

    HRESULT __cdecl FinalizeVBAndPointReps(
        _In_reads_bytes_(nVerts*stride) const void* vbin, _In_ size_t stride, _In_ size_t nVerts,
        _In_reads_(nVerts) const uint32_t* prin,
        _In_reads_opt_(nDupVerts) const uint32_t* dupVerts, _In_ size_t nDupVerts,
        _In_reads_opt_(nVerts + nDupVerts) const uint32_t* vertexRemap,
        _Out_writes_bytes_((nVerts + nDupVerts)*stride) void* vbout,
        _Out_writes_(nVerts + nDupVerts) uint32_t* prout) noexcept;
    HRESULT __cdecl FinalizeVBAndPointReps(
        _Inout_updates_bytes_all_(nVerts*stride) void* vb, _In_ size_t stride, _In_ size_t nVerts,
        _Inout_updates_all_(nVerts) uint32_t* pointRep,
        _In_reads_(nVerts) const uint32_t* vertexRemap) noexcept;
        // Applies a vertex remap and/or a vertex duplication set to a vertex buffer and point representatives

    HRESULT __cdecl CompactVB(
        _In_reads_bytes_(nVerts*stride) const void* vbin, _In_ size_t stride, _In_ size_t nVerts,
        _In_ size_t trailingUnused,
        _In_reads_opt_(nVerts) const uint32_t* vertexRemap,
        _Out_writes_bytes_((nVerts - trailingUnused)*stride) void* vbout) noexcept;
        // Applies a vertex remap which contains a known number of unused entries at the end

    //---------------------------------------------------------------------------------
    // Meshlet Generation

    constexpr size_t MESHLET_DEFAULT_MAX_VERTS = 128u;
    constexpr size_t MESHLET_DEFAULT_MAX_PRIMS = 128u;

    constexpr size_t MESHLET_MINIMUM_SIZE = 32u;
    constexpr size_t MESHLET_MAXIMUM_SIZE = 256u;

    enum MESHLET_FLAGS : unsigned long
    {
        MESHLET_DEFAULT = 0x0,

        MESHLET_WIND_CW = 0x1,
        // Vertices are clock-wise (defaults to CCW)
    };

    struct Meshlet
    {
        uint32_t VertCount;
        uint32_t VertOffset;
        uint32_t PrimCount;
        uint32_t PrimOffset;
    };

    struct MeshletTriangle
    {
        uint32_t i0 : 10;
        uint32_t i1 : 10;
        uint32_t i2 : 10;
    };

    struct CullData
    {
        DirectX::BoundingSphere             BoundingSphere; // xyz = center, w = radius
        DirectX::PackedVector::XMUBYTEN4    NormalCone;     // xyz = axis, w = -cos(a + 90)
        float                               ApexOffset;     // apex = center - axis * offset
    };

    HRESULT __cdecl ComputeMeshlets(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_reads_opt_(nFaces * 3) const uint32_t* adjacency,
        _Inout_ std::vector<Meshlet>& meshlets,
        _Inout_ std::vector<uint8_t>& uniqueVertexIB,
        _Inout_ std::vector<MeshletTriangle>& primitiveIndices,
        _In_ size_t maxVerts = MESHLET_DEFAULT_MAX_VERTS, _In_ size_t maxPrims = MESHLET_DEFAULT_MAX_PRIMS);
    HRESULT __cdecl ComputeMeshlets(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_reads_opt_(nFaces * 3) const uint32_t* adjacency,
        _Inout_ std::vector<Meshlet>& meshlets,
        _Inout_ std::vector<uint8_t>& uniqueVertexIB,
        _Inout_ std::vector<MeshletTriangle>& primitiveIndices,
        _In_ size_t maxVerts = MESHLET_DEFAULT_MAX_VERTS, _In_ size_t maxPrims = MESHLET_DEFAULT_MAX_PRIMS);
        // Generates meshlets for a single subset mesh

    HRESULT __cdecl ComputeMeshlets(
        _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_reads_(nSubsets) const std::pair<size_t, size_t>* subsets, _In_ size_t nSubsets,
        _In_reads_opt_(nFaces * 3) const uint32_t* adjacency,
        _Inout_ std::vector<Meshlet>& meshlets,
        _Inout_ std::vector<uint8_t>& uniqueVertexIB,
        _Inout_ std::vector<MeshletTriangle>& primitiveIndices,
        _Out_writes_(nSubsets) std::pair<size_t, size_t>* meshletSubsets,
        _In_ size_t maxVerts = MESHLET_DEFAULT_MAX_VERTS, _In_ size_t maxPrims = MESHLET_DEFAULT_MAX_PRIMS);
    HRESULT __cdecl ComputeMeshlets(
        _In_reads_(nFaces * 3) const uint32_t* indices, _In_ size_t nFaces,
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_reads_(nSubsets) const std::pair<size_t, size_t>* subsets, _In_ size_t nSubsets,
        _In_reads_opt_(nFaces * 3) const uint32_t* adjacency,
        _Inout_ std::vector<Meshlet>& meshlets,
        _Inout_ std::vector<uint8_t>& uniqueVertexIB,
        _Inout_ std::vector<MeshletTriangle>& primitiveIndices,
        _Out_writes_(nSubsets) std::pair<size_t, size_t>* meshletSubsets,
        _In_ size_t maxVerts = MESHLET_DEFAULT_MAX_VERTS, _In_ size_t maxPrims = MESHLET_DEFAULT_MAX_PRIMS);
        // Generates meshlets for a mesh with several face subsets

    HRESULT __cdecl ComputeCullData(
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_reads_(nMeshlets) const Meshlet* meshlets, _In_ size_t nMeshlets,
        _In_reads_(nVertIndices) const uint16_t* uniqueVertexIndices, _In_ size_t nVertIndices,
        _In_reads_(nPrimIndices) const MeshletTriangle* primitiveIndices, _In_ size_t nPrimIndices,
        _Out_writes_(nMeshlets) CullData* cullData,
        _In_ MESHLET_FLAGS flags = MESHLET_DEFAULT) noexcept;
    HRESULT __cdecl ComputeCullData(
        _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
        _In_reads_(nMeshlets) const Meshlet* meshlets, _In_ size_t nMeshlets,
        _In_reads_(nVertIndices) const uint32_t* uniqueVertexIndices, _In_ size_t nVertIndices,
        _In_reads_(nPrimIndices) const MeshletTriangle* primitiveIndices, _In_ size_t nPrimIndices,
        _Out_writes_(nMeshlets) CullData* cullData,
        _In_ MESHLET_FLAGS flags = MESHLET_DEFAULT) noexcept;
        // Computes culling data for each input meshlet

    //---------------------------------------------------------------------------------
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#endif

#include "DirectXMesh.inl"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} // namespace
