// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Custom node archetype flags.
    /// </summary>
    [Flags]
    [HideInEditor]
    public enum NodeFlags
    {
        /// <summary>
        /// Nothing at all. Nothing but thieves.
        /// </summary>
        None = 0,

        /// <summary>
        /// Don't adds a close button.
        /// </summary>
        NoCloseButton = 1,

        /// <summary>
        /// Node should use dependant and independent boxes types.
        /// TODO: unused UseDependantBoxes flag, remove it
        /// </summary>
        UseDependantBoxes = 2,

        /// <summary>
        /// Node cannot be spawned via GUI interface (but can be pasted).
        /// </summary>
        NoSpawnViaGUI = 4,

        /// <summary>
        /// Node can be used in the material graphs.
        /// </summary>
        MaterialGraph = 8,

        /// <summary>
        /// Node can be used in the particle emitter graphs.
        /// </summary>
        ParticleEmitterGraph = 16,

        /// <summary>
        /// Disables removing that node from the graph.
        /// </summary>
        NoRemove = 32,

        /// <summary>
        /// Node can be used in the animation graphs.
        /// </summary>
        AnimGraph = 64,

        /// <summary>
        /// Disables moving node (by user).
        /// </summary>
        NoMove = 128,

        /// <summary>
        /// Node can be used in the visual script graphs.
        /// </summary>
        VisualScriptGraph = 256,

        /// <summary>
        /// Node cannot be spawned via copy-paste (can be added only from code).
        /// </summary>
        NoSpawnViaPaste = 512,

        /// <summary>
        /// Node can be used in the Behavior Tree graphs.
        /// </summary>
        BehaviorTreeGraph = 1024,

        /// <summary>
        /// Node can have different amount of items in values array.
        /// </summary>
        VariableValuesSize = 2048,

        /// <summary>
        /// Node can be used in the all visual graphs.
        /// </summary>
        AllGraphs = MaterialGraph | ParticleEmitterGraph | AnimGraph | VisualScriptGraph | BehaviorTreeGraph,
    }
}
