// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial class Script
    {
        /// <summary>
        /// Gets the scene object which contains this script.
        /// </summary>
        [HideInEditor, NoSerialize]
        public Scene Scene
        {
            get
            {
                var parent = Actor;
                return parent ? parent.Scene : null;
            }
        }

        /// <summary>
        /// Gets or sets the world space transformation of the actors owning this script.
        /// </summary>
        [HideInEditor, NoSerialize, NoAnimate]
        public Transform Transform
        {
            get => Actor.Transform;
            set => Actor.Transform = value;
        }

        /// <summary>
        /// Gets or sets the local space transformation of the actors owning this script.
        /// </summary>
        [HideInEditor, NoSerialize, NoAnimate]
        public Transform LocalTransform
        {
            get => Actor.LocalTransform;
            set => Actor.LocalTransform = value;
        }
    }
}
