//-------------------------------------------------------------------------------------
// UVAtlas
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=512686
//-------------------------------------------------------------------------------------

#pragma once

#ifdef _WIN32
#ifdef _GAMING_XBOX_SCARLETT
#pragma warning(push)
#pragma warning(disable: 5204 5249)
#include <d3d12_xs.h>
#pragma warning(pop)
#elif defined(_GAMING_XBOX)
#pragma warning(push)
#pragma warning(disable: 5204)
#include <d3d12_x.h>
#pragma warning(pop)
#elif defined(_XBOX_ONE) && defined(_TITLE)
#error This library no longer supports legacy Xbox One XDK
#else
#include <Windows.h>
#ifdef USING_DIRECTX_HEADERS
#include <directx/dxgiformat.h>
#else
#include <dxgiformat.h>
#endif
#endif
#else // !WIN32
#include <directx/dxgiformat.h>
#include <wsl/winadapter.h>
#endif

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <DirectXMath.h>

#define UVATLAS_VERSION 187


namespace DirectX
{
    // Output vertex format
    struct UVAtlasVertex
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 uv;
    };

    // UVATLAS_IMT_WRAP_U means the texture wraps in the U direction
    // UVATLAS_IMT_WRAP_V means the texture wraps in the V direction
    // UVATLAS_IMT_WRAP_UV means the texture wraps in both directions
    enum UVATLAS_IMT : unsigned int
    {
        UVATLAS_IMT_DEFAULT = 0x00,
        UVATLAS_IMT_WRAP_U = 0x01,
        UVATLAS_IMT_WRAP_V = 0x02,
        UVATLAS_IMT_WRAP_UV = 0x03,
    };

    // These options are only valid for UVAtlasCreate and UVAtlasPartition
    // UVATLAS_DEFAULT - Meshes with more than 25k faces go through fast, meshes with fewer than 25k faces go through quality
    // UVATLAS_GEODESIC_FAST - Uses approximations to improve charting speed at the cost of added stretch or more charts.
    // UVATLAS_GEODESIC_QUALITY - Provides better quality charts, but requires more time and memory than fast.
    enum UVATLAS : unsigned int
    {
        UVATLAS_DEFAULT = 0x00,
        UVATLAS_GEODESIC_FAST = 0x01,
        UVATLAS_GEODESIC_QUALITY = 0x02,
        UVATLAS_LIMIT_MERGE_STRETCH = 0x04,
        UVATLAS_LIMIT_FACE_STRETCH = 0x08,
    };

    constexpr float UVATLAS_DEFAULT_CALLBACK_FREQUENCY = 0.0001f;

    //============================================================================
    //
    // UVAtlas apis
    //
    //============================================================================

    // This function creates atlases for meshes. There are two modes of operation,
    // either based on the number of charts, or the maximum allowed stretch. If the
    // maximum allowed stretch is 0, then each triangle will likely be in its own
    // chart.

    //  maxChartNumber - The maximum number of charts required for the atlas.
    //                   If this is 0, it will be parameterized based solely on
    //                   stretch.
    //  maxStretch - The maximum amount of stretch, if 0, no stretching is allowed,
    //               if 1, then any amount of stretching is allowed.
    //  gutter - The minimum distance, in texels between two charts on the atlas.
    //           this gets scaled by the width, so if gutter is 2.5, and it is
    //           used on a 512x512 texture, then the minimum distance will be
    //           2.5 / 512 in u-v space.
    //  falseEdgeAdjacency - a pointer to an array with 3 uint32_t per face, indicating
    //                       at each face, whether an edge is a false edge or not (using
    //                       the same ordering as the adjacency data structure). If this
    //                       is nullptr, then it is assumed that there are no false edges. If
    //                       not nullptr, then a non-false edge is indicated by -1 and a false
    //                       edge is indicated by any other value (it is not required, but
    //                       it may be useful for the caller to use the original adjacency
    //                       value). This allows you to parameterize a mesh of quads, and
    //                       the edges down the middle of each quad will not be cut when
    //                       parameterizing the mesh.
    //  pIMTArray - a pointer to an array with 3 floats per face, describing the
    //               integrated metric tensor for that face. This lets you control
    //               the way this triangle may be stretched in the atlas. The IMT
    //               passed in will be 3 floats (a,b,c) and specify a symmetric
    //               matrix (a b) that, given a vector (s,t), specifies the
    //                      (b c)
    //               distance between a vector v1 and a vector v2 = v1 + (s,t) as
    //               sqrt((s, t) * M * (s, t)^T).
    //               In other words, this lets one specify the magnitude of the
    //               stretch in an arbitrary direction in u-v space. For example
    //               if a = b = c = 1, then this scales the vector (1,1) by 2, and
    //               the vector (1,-1) by 0. Note that this is multiplying the edge
    //               length by the square of the matrix, so if you want the face to
    //               stretch to twice its
    //               size with no shearing, the IMT value should be (2, 0, 2), which
    //               is just the identity matrix times 2.
    //               Note that this assumes you have an orientation for the triangle
    //               in some 2-D space. For UVAtlas, this space is created by
    //               letting S be the direction from the first to the second
    //               vertex, and T be the cross product between the normal and S.
    //  statusCallback - Since the atlas creation process can be very CPU intensive,
    //                   this allows the programmer to specify a function to be called
    //                   periodically.
    //  callbackFrequency - This lets you specify how often the callback will be called.
    //  options - A combination of flags in the UVATLAS enum
    //  pvFacePartitioning - A pointer to a location to store a pointer for an array,
    //                       one uint32_t per face, giving the final partitioning
    //                       created by the atlasing algorithm.
    //  pvVertexRemapArray - A uint32_t to a location to store a pointer for an array,
    //                       one uint32_t per vertex, giving the vertex it was copied
    //                       from, if any vertices needed to be split.
    //  maxStretchOut - The maximum stretch resulting from the atlasing algorithm.
    //  numChartsOut - A location to store the number of charts created, or if the
    //                 maximum number of charts was too low, this gives the minimum
    //                 number of charts needed to create an atlas.

    HRESULT __cdecl UVAtlasCreate(
        _In_reads_(nVerts)                  const XMFLOAT3* positions,
        _In_                                size_t nVerts,
        _When_(indexFormat == DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint16_t)))
        _When_(indexFormat != DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint32_t))) const void* indices,
        _In_                                DXGI_FORMAT indexFormat,
        _In_                                size_t nFaces,
        _In_                                size_t maxChartNumber,
        _In_                                float maxStretch,
        _In_                                size_t width,
        _In_                                size_t height,
        _In_                                float gutter,
        _In_reads_(nFaces * 3)                const uint32_t* adjacency,
        _In_reads_opt_(nFaces * 3)            const uint32_t* falseEdgeAdjacency,
        _In_reads_opt_(nFaces * 3)            const float* pIMTArray,
        _In_                                std::function<HRESULT __cdecl(float percentComplete)> statusCallBack,
        _In_                                float callbackFrequency,
        _In_                                UVATLAS options,
        _Inout_ std::vector<UVAtlasVertex>& vMeshOutVertexBuffer,
        _Inout_ std::vector<uint8_t>& vMeshOutIndexBuffer,
        _Inout_opt_ std::vector<uint32_t>* pvFacePartitioning = nullptr,
        _Inout_opt_ std::vector<uint32_t>* pvVertexRemapArray = nullptr,
        _Out_opt_                           float* maxStretchOut = nullptr,
        _Out_opt_                           size_t* numChartsOut = nullptr);

    // This has the same exact arguments as Create, except that it does not perform the
    // final packing step. This method allows one to get a partitioning out, and possibly
    // modify it before sending it to be repacked. Note that if you change the
    // partitioning, you'll also need to calculate new texture coordinates for any faces
    // that have switched charts.
    //
    // The partition result adjacency output parameter is meant to be passed to the
    // UVAtlasPack function, this adjacency cuts edges that are between adjacent
    // charts, and also can include cuts inside of a chart in order to make it
    // equivalent to a disc. For example:
    //
    // _______
    // | ___ |
    // | |_| |
    // |_____|
    //
    // In order to make this equivalent to a disc, we would need to add a cut, and it
    // Would end up looking like:
    // _______
    // | ___ |
    // | |_|_|
    // |_____|
    //

    HRESULT __cdecl UVAtlasPartition(
        _In_reads_(nVerts)          const XMFLOAT3* positions,
        _In_                        size_t nVerts,
        _When_(indexFormat == DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint16_t)))
        _When_(indexFormat != DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint32_t))) const void* indices,
        _In_                        DXGI_FORMAT indexFormat,
        _In_                        size_t nFaces,
        _In_                        size_t maxChartNumber,
        _In_                        float maxStretch,
        _In_reads_(nFaces * 3)      const uint32_t* adjacency,
        _In_reads_opt_(nFaces * 3)  const uint32_t* falseEdgeAdjacency,
        _In_reads_opt_(nFaces * 3)  const float* pIMTArray,
        _In_                        std::function<HRESULT __cdecl(float percentComplete)> statusCallBack,
        _In_                        float callbackFrequency,
        _In_                        UVATLAS options,
        _Inout_                     std::vector<UVAtlasVertex>& vMeshOutVertexBuffer,
        _Inout_                     std::vector<uint8_t>& vMeshOutIndexBuffer,
        _Inout_opt_                 std::vector<uint32_t>* pvFacePartitioning,
        _Inout_opt_                 std::vector<uint32_t>* pvVertexRemapArray,
        _Inout_                     std::vector<uint32_t>& vPartitionResultAdjacency,
        _Out_opt_                   float* maxStretchOut = nullptr,
        _Out_opt_                   size_t* numChartsOut = nullptr);

    // This takes the face partitioning result from Partition and packs it into an
    // atlas of the given size. pPartitionResultAdjacency should be derived from
    // the adjacency returned from the partition step.
    HRESULT __cdecl UVAtlasPack(
        _Inout_                 std::vector<UVAtlasVertex>& vMeshVertexBuffer,
        _Inout_                 std::vector<uint8_t>& vMeshIndexBuffer,
        _In_                    DXGI_FORMAT indexFormat,
        _In_                    size_t width,
        _In_                    size_t height,
        _In_                    float gutter,
        _In_                    const std::vector<uint32_t>& vPartitionResultAdjacency,
        _In_                    std::function<HRESULT __cdecl(float percentComplete)> statusCallBack,
        _In_                    float callbackFrequency);


    //============================================================================
    //
    // IMT Calculation apis
    //
    // These functions all compute the Integrated Metric Tensor for use in the
    // UVAtlas API. They all calculate the IMT with respect to the canonical
    // triangle, where the coordinate system is set up so that the u axis goes
    // from vertex 0 to 1 and the v axis is N x u. So, for example, the second
    // vertex's canonical uv coordinates are (d,0) where d is the distance between
    // vertices 0 and 1. This way the IMT does not depend on the parameterization
    // of the mesh, and if the signal over the surface doesn't change, then
    // the IMT doesn't need to be recalculated.
    //============================================================================

    // This function is used to calculate the IMT from per vertex data. It sets
    // up a linear system over the triangle, solves for the jacobian J, then
    // constructs the IMT from that (J^TJ).
    // This function allows you to calculate the IMT based off of any value in a
    // mesh (color, normal, etc) by specifying the correct stride of the array.
    // The IMT computed will cause areas of the mesh that have similar values to
    // take up less space in the texture.
    //
    // pVertexSignal    - A float array of size signalStride * nVerts
    // signalDimension  - How many floats per vertex to use in calculating the IMT.
    // signalStride     - The number of bytes per vertex in the array. This must be
    //                    a multiple of sizeof(float)
    // pIMTArray        - An array of 3 * nFaces floats for the result

    HRESULT __cdecl UVAtlasComputeIMTFromPerVertexSignal(
        _In_reads_(nVerts)                  const XMFLOAT3* positions,
        _In_                                size_t nVerts,
        _When_(indexFormat == DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint16_t)))
        _When_(indexFormat != DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint32_t))) const void* indices,
        _In_                                DXGI_FORMAT indexFormat,
        _In_                                size_t nFaces,
        _In_reads_(signalStride* nVerts)     const float* pVertexSignal,
        _In_                                size_t signalDimension,
        _In_                                size_t signalStride,
        _In_                                std::function<HRESULT __cdecl(float percentComplete)> statusCallBack,
        _Out_writes_(nFaces * 3)            float* pIMTArray);

    // This function is used to calculate the IMT from data that varies over the
    // surface of the mesh (generally at a higher frequency than vertex data).
    // This function requires the mesh to already be parameterized (so it already
    // has texture coordinates). It allows the user to define a signal arbitrarily
    // over the surface of the mesh.
    //
    // signalDimension  - How many components there are in the signal.
    // maxUVDistance    - The subdivision will continue until the distance between
    //                    all vertices is at most maxUVDistance.
    // signalCallback  - The callback to use to get the signal.
    //                   uv -  The texture coordinate for the vertex.
    //                   primitiveID - Face ID of the triangle on which to compute the signal.
    //                   signalDimension - The number of floats to store in sSignalOut.
    //                   userData - The userData pointer passed in to ComputeIMTFromSignal
    //                   signalOut - A pointer to where to store the signal data.
    // userData        - A pointer that will be passed in to the callback.
    // pIMTArray        - An array of 3 * nFaces floats for the result
    HRESULT __cdecl UVAtlasComputeIMTFromSignal(
        _In_reads_(nVerts)                  const XMFLOAT3* positions,
        _In_reads_(nVerts)                  const XMFLOAT2* texcoords,
        _In_                                size_t nVerts,
        _When_(indexFormat == DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint16_t)))
        _When_(indexFormat != DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint32_t))) const void* indices,
        _In_                                DXGI_FORMAT indexFormat,
        _In_                                size_t nFaces,
        _In_                                size_t signalDimension,
        _In_                                float maxUVDistance,
        _In_                                std::function<HRESULT __cdecl(const DirectX::XMFLOAT2* uv, size_t primitiveID, size_t signalDimension, void* userData, float* signalOut)>
        signalCallback,
        _In_opt_                            void* userData,
        _In_                                std::function<HRESULT __cdecl(float percentComplete)> statusCallBack,
        _Out_writes_(nFaces * 3)            float* pIMTArray);

    // This function is used to calculate the IMT from texture data. Given a texture
    // that maps over the surface of the mesh, the algorithm computes the IMT for
    // each face. This will cause large areas that are very similar to take up less
    // room when parameterized with UVAtlas. The texture is assumed to be
    // interpolated over the mesh bilinearly.
    //
    // pTexture         - The texture to load data from (4 floats per texel)
    // options          - Combination of one or more UVATLAS_IMT flags.
    // pIMTArray        - An array of 3 * nFaces floats for the result
    HRESULT __cdecl UVAtlasComputeIMTFromTexture(
        _In_reads_(nVerts)                  const XMFLOAT3* positions,
        _In_reads_(nVerts)                  const XMFLOAT2* texcoords,
        _In_                                size_t nVerts,
        _When_(indexFormat == DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint16_t)))
        _When_(indexFormat != DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint32_t))) const void* indices,
        _In_                                DXGI_FORMAT indexFormat,
        _In_                                size_t nFaces,
        _In_reads_(width* height * 4)          const float* pTexture,
        _In_                                size_t width,
        _In_                                size_t height,
        _In_                                UVATLAS_IMT options,
        _In_                                std::function<HRESULT __cdecl(float percentComplete)> statusCallBack,
        _Out_writes_(nFaces * 3)            float* pIMTArray);

    // This function is very similar to UVAtlasComputeIMTFromTexture, but it can
    // calculate higher dimensional values than 4.
    //
    // pTexelSignal     - a pointer to a float array of size width*height*nComponents
    // width            - The width of the texture
    // height           - The height of the texture
    // signalDimension  - The number of floats per texel in the signal
    // nComponents      - The number of floats in each texel
    // options          - Combination of one or more UVATLAS_IMT flags
    // pIMTArray        - An array of 3 * nFaces floats for the result
    HRESULT __cdecl UVAtlasComputeIMTFromPerTexelSignal(
        _In_reads_(nVerts)                      const XMFLOAT3* positions,
        _In_reads_(nVerts)                      const XMFLOAT2* texcoords,
        _In_                                    size_t nVerts,
        _When_(indexFormat == DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint16_t)))
        _When_(indexFormat != DXGI_FORMAT_R16_UINT, _In_reads_bytes_(nFaces * 3 * sizeof(uint32_t))) const void* indices,
        _In_                                    DXGI_FORMAT indexFormat,
        _In_                                    size_t nFaces,
        _In_reads_(width* height* nComponents)    const float* pTexelSignal,
        _In_                                    size_t width,
        _In_                                    size_t height,
        _In_                                    size_t signalDimension,
        _In_                                    size_t nComponents,
        _In_                                    UVATLAS_IMT options,
        _In_                                    std::function<HRESULT __cdecl(float percentComplete)> statusCallBack,
        _Out_writes_(nFaces * 3)                float* pIMTArray);

    // This function is for applying the a vertex remap array from UVAtlasCreate/UVAtlasPartition to a vertex buffer
    //
    // vbin         - This is the original vertex buffer and is nVerts*stride in size
    // vbout        - This is the output vertex buffer and is nNewVerts*stride in size
    // nNewVerts    - This should be >= nVerts
    HRESULT __cdecl UVAtlasApplyRemap(
        _In_reads_bytes_(nVerts* stride)        const void* vbin,
        _In_                                    size_t stride,
        _In_                                    size_t nVerts,
        _In_                                    size_t nNewVerts,
        _In_reads_(nNewVerts)                   const uint32_t* vertexRemap,
        _Out_writes_bytes_(nNewVerts* stride)   void* vbout) noexcept;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-dynamic-exception-spec"
#endif

    DEFINE_ENUM_FLAG_OPERATORS(UVATLAS_IMT);
    DEFINE_ENUM_FLAG_OPERATORS(UVATLAS);

#ifdef __clang__
#pragma clang diagnostic pop
#endif
}
