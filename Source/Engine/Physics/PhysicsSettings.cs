// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using FlaxEngine;
using System.Linq;

namespace FlaxEditor.Content.Settings
{
    partial class PhysicsSettings
    {
        /// <summary>
        /// The collision layers masks. Used to define layer-based collision detection.
        /// </summary>
#if FLAX_EDITOR
        [EditorOrder(1040), EditorDisplay("Layers Matrix"), CustomEditor(typeof(CustomEditors.Dedicated.LayersMatrixEditor))]
#endif
        public uint[] LayerMasks = new uint[32];

        /// <summary>
        /// Initializes a new instance of the <see cref="PhysicsSettings"/> class.
        /// </summary>
        public PhysicsSettings()
        {
            LayerMasks[i] = Enumerable.Repeat(uint.MaxValue, 32).ToArray();
        }
    }
}
