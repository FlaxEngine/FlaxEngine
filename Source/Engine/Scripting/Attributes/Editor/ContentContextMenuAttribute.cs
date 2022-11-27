using System;

namespace FlaxEngine
{
    /// <summary>
    /// This attribute is used to show content items that can be created in the content browser context menu. Separate the subcontext menus with a /. 
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Class)]
    public class ContentContextMenuAttribute : Attribute
    {
        /// <summary>
        /// The path to be used in the context menu
        /// </summary>
        public string Path;

        /// <summary>
        /// Initializes a new instance of the  <see cref="ContentContextMenuAttribute"/> class.
        /// </summary>
        /// <param name="path">The path to use to create the context menu.</param>
        public ContentContextMenuAttribute(string path)
        {
            Path = path;
        }
    }
}
