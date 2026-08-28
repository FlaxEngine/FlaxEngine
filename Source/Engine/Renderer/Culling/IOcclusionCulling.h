// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/BoundingBox.h"

struct GeometryDrawState;
struct RenderContext;
struct RenderContextBatch;

/// <summary>
/// Interface for occlusion querying and culling systems. Performs visibility checks for the scene objects (incl. meshes and lights).
/// Implementations can use hardware occlusion queries, software rasterization, Hi-Z tests, or any other method to determine if an object is visible in the current view frustum and not occluded by other geometry.
/// </summary>
/// <remarks>Can be implemented only in the native code (C++) but also used in scripting for custom objects culling.</remarks>
API_INTERFACE() class FLAXENGINE_API IOcclusionCulling
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(IOcclusionCulling);
    virtual ~IOcclusionCulling() = default;

    /// <summary>
    /// Checks if the culling system is supported on the current platform and hardware.
    /// </summary>
    virtual bool IsSupported() { return true; }

    /// <summary>
    /// Frame begin event. Called before the drawing to prepare the culling system for the new frame.
    /// </summary>
    virtual void BeginFrame(const RenderContext& renderContext) {}

    /// <summary>
    /// Submits occlusion queries or performs the culling operations. Called after occluders depth drawing (depth from GBufferPass or DepthPrePass). Can be used to submit occlusion queries for the scene objects (eg., using GPU occlusion queries) or generate a Hi-Z buffer.
    /// </summary>
    virtual void Submit(const RenderContext& renderContext) {}

    /// <summary>
    /// Frame end event. Called after the drawing to prepare the culling system for the new frame.
    /// </summary>
    virtual void EndFrame(const RenderContext& renderContext) {}

    /// <summary>
    /// Object bounds visibility check. Returns true if the object is visible (not occluded by other geometry).
    /// Works only for CPU-side culling (or with delayed GPU-readback).
    /// GPU-based culling uses indirect draw arguments to handle conditional drawing.
    /// Usually called from multiple threads at once (async) when rendering scene.
    /// </summary>
    /// <param name="bounds">The bounds of the object to check.</param>
    /// <param name="cullingId">The unique identifier of the visibility query - stable for the same object. Set to 0 by default (as invalid ID), will be assigned internally by the culling system.</param>
    /// <returns>True if object can be rendered (is visible or visibility will be calculated on GPU), otherwise false.</returns>
    API_FUNCTION(Sealed) virtual bool IsVisible(const BoundingBox& bounds, API_PARAM(Ref) uint32& cullingId) { return true; }

    /// <summary>
    /// Object bounds visibility check. Returns true if the object is visible (not occluded by other geometry).
    /// Works only for CPU-side culling (or with delayed GPU-readback).
    /// GPU-based culling uses indirect draw arguments to handle conditional drawing.
    /// Usually called from multiple threads at once (async) when rendering scene.
    /// </summary>
    /// <param name="bounds">The bounds of the object to check.</param>
    /// <param name="drawState">The geometry drawing state - allocated within RenderBuffers for a single model actor. Contains CullingId field that will be assigned and used internally by the culling system.</param>
    /// <returns>True if object can be rendered (is visible or visibility will be calculated on GPU), otherwise false.</returns>
    virtual bool IsVisible(const BoundingBox& bounds, GeometryDrawState& drawState) { return true; }
};
