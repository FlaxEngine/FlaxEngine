// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Brush.h"

class Scene;

namespace CSG
{
    class RawData;

#if COMPILE_WITH_CSG_BUILDER

    /// <summary>
    /// CSG geometry builder
    /// </summary>
    class Builder
    {
    public:

        /// <summary>
        /// Action fired when any CSG brush on scene gets edited (different dimensions or transformation etc.)
        /// </summary>
        static Delegate<Brush*> OnBrushModified;

    public:

        static bool IsActive();

        /// <summary>
        /// Build CSG geometry for the given scene.
        /// </summary>
        /// <param name="scene">The scene.</param>
        /// <param name="timeoutMs">The timeout to wait before building CSG (in milliseconds).</param>
        static void Build(Scene* scene, float timeoutMs = 50);
    };

#endif
};
