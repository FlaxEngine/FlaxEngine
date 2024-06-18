// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

@0// SDF Reflections: Defines
#define USE_GLOBAL_SURFACE_ATLAS 1
@1// SDF Reflections: Includes
#include "./Flax/GlobalSignDistanceField.hlsl"
#include "./Flax/GI/GlobalSurfaceAtlas.hlsl"
@2// SDF Reflections: Constants
GlobalSDFData GlobalSDF;
GlobalSurfaceAtlasData GlobalSurfaceAtlas;
@3// SDF Reflections: Resources
Texture3D<float> GlobalSDFTex : register(t__SRV__);
Texture3D<float> GlobalSDFMip : register(t__SRV__);
ByteAddressBuffer GlobalSurfaceAtlasChunks : register(t__SRV__);
ByteAddressBuffer RWGlobalSurfaceAtlasCulledObjects : register(t__SRV__);
Buffer<float4> GlobalSurfaceAtlasObjects : register(t__SRV__);
Texture2D GlobalSurfaceAtlasDepth : register(t__SRV__);
Texture2D GlobalSurfaceAtlasTex : register(t__SRV__);
@4// SDF Reflections: Utilities
bool TraceSDFSoftwareReflections(GBufferSample gBuffer, float3 reflectWS, out float4 surfaceAtlas)
{
	GlobalSDFTrace sdfTrace;
    float maxDistance = 100000;
    float selfOcclusionBias = GlobalSDF.CascadeVoxelSize[0];
    sdfTrace.Init(gBuffer.WorldPos + gBuffer.Normal * selfOcclusionBias, reflectWS, 0.0f, maxDistance);
    GlobalSDFHit sdfHit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, sdfTrace);
    if (sdfHit.IsHit())
    {
        float3 hitPosition = sdfHit.GetHitPosition(sdfTrace);
        float surfaceThreshold = GetGlobalSurfaceAtlasThreshold(GlobalSDF, sdfHit);
        surfaceAtlas = SampleGlobalSurfaceAtlas(GlobalSurfaceAtlas, GlobalSurfaceAtlasChunks, RWGlobalSurfaceAtlasCulledObjects, GlobalSurfaceAtlasObjects, GlobalSurfaceAtlasDepth, GlobalSurfaceAtlasTex, hitPosition, -reflectWS, surfaceThreshold);
        
        return true;
    }

    return false;
}

@5// SDF Reflections: Shaders
