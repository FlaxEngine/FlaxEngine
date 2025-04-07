// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Surface.Archetypes;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The Visject Surface implementation for the animation graph functions editor.
    /// </summary>
    /// <seealso cref="AnimGraphSurface" />
    /// <seealso cref="Function.IFunctionSurface" />
    [HideInEditor]
    public class AnimationGraphFunctionSurface : AnimGraphSurface, Function.IFunctionSurface
    {
        private static readonly Type[] AnimationGraphFunctionTypes =
        {
            typeof(bool),
            typeof(int),
            typeof(float),
            typeof(Float2),
            typeof(Float3),
            typeof(Float4),
            typeof(Vector2),
            typeof(Vector3),
            typeof(Vector4),
            typeof(void),
        };

        /// <inheritdoc />
        public AnimationGraphFunctionSurface(IVisjectSurfaceOwner owner, Action onSave, FlaxEditor.Undo undo)
        : base(owner, onSave, undo)
        {
        }

        /// <inheritdoc />
        public override bool CanUseNodeType(GroupArchetype groupArchetype, NodeArchetype nodeArchetype)
        {
            if (nodeArchetype.Title == "Function Input")
                return true;

            // Allow to use Function Output only in the root graph (not in state machine sub-graphs)
            if (Context == RootContext && nodeArchetype.Title == "Function Output")
                return true;

            return base.CanUseNodeType(groupArchetype, nodeArchetype);
        }

        /// <inheritdoc />
        public Type[] FunctionTypes => AnimationGraphFunctionTypes;
    }
}
