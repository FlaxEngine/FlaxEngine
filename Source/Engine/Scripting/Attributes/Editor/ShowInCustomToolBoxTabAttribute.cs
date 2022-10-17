using System;

namespace FlaxEngine
{
    /// <summary>
    /// Allows to add an actor class to the Custom tab in the tool box. This will only will show Actors.
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Class)]
    public class ShowInCustomToolBoxTabAttribute: Attribute
    {
        
    }
}
