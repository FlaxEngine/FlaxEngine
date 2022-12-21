using System;

namespace FlaxEngine
{
    /// <summary>
    /// This attribute is used to show actors that can be created in the scene and prefab context menus. Separate the subcontext menus with a /. 
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Class)]
    public class ActorContextMenuAttribute : Attribute
    {
        /// <summary>
        /// The path to be used in the context menu
        /// </summary>
        public string Path;

        /// <summary>
        /// Initializes a new instance of the  <see cref="ActorContextMenuAttribute"/> class.
        /// </summary>
        /// <param name="path">The path to use to create the context menu.</param>
        public ActorContextMenuAttribute(string path)
        {
            Path = path;
        }
    }
}
