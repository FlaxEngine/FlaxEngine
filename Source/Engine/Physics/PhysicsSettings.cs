// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

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
            for (int i = 0; i < 32; i++)
            {
                LayerMasks[i] = uint.MaxValue;
            }
        }
    }
}
