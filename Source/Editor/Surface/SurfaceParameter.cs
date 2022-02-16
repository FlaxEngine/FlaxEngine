// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Represents parameter in the Surface.
    /// </summary>
    [HideInEditor]
    public class SurfaceParameter
    {
        /// <summary>
        /// The default prefix for drag data used for <see cref="FlaxEditor.Surface.SurfaceParameter"/>.
        /// </summary>
        public const string DragPrefix = "SURFPARAM!?";

        /// <summary>
        /// Parameter type
        /// </summary>
        [NoSerialize, HideInEditor]
        public ScriptType Type;

        /// <summary>
        /// Parameter unique ID
        /// </summary>
        public Guid ID;

        /// <summary>
        /// Parameter name
        /// </summary>
        public string Name;

        /// <summary>
        /// True if is exposed outside
        /// </summary>
        public bool IsPublic;

        /// <summary>
        /// Parameter value
        /// </summary>
        public object Value;

        /// <summary>
        /// The metadata.
        /// </summary>
        [NoSerialize, HideInEditor]
        public readonly SurfaceMeta Meta = new SurfaceMeta();

        /// <summary>
        /// Creates the new parameter of the given type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="name">The name.</param>
        /// <returns>The created parameter.</returns>
        public static SurfaceParameter Create(ScriptType type, string name)
        {
            return new SurfaceParameter
            {
                ID = Guid.NewGuid(),
                IsPublic = true,
                Name = name,
                Type = type,
                Value = TypeUtils.GetDefaultValue(type),
            };
        }
    }
}
