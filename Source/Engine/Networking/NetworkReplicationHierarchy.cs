// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.Networking
{
    partial struct NetworkReplicationHierarchyObject
    {
        /// <summary>
        /// Gets the actors context (object itself or parent actor).
        /// </summary>
        public Actor Actor
        {
            get
            {
                var actor = Object as Actor;
                if (actor == null)
                {
                    var sceneObject = Object as SceneObject;
                    if (sceneObject != null)
                        actor = sceneObject.Parent;
                }
                return actor;
            }
        }
    }
}
