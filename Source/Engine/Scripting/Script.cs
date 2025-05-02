// Copyright (c) Wojciech Figat. All rights reserved.

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
        /// Gets value indicating if the actor owning the script is in a scene.
        /// </summary>
        [HideInEditor, NoSerialize]
        public bool HasScene => Actor?.HasScene ?? false; 

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
