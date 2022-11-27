using System;

namespace FlaxEngine
{
    /// <summary>
    /// This attribute is used to show actors that can be created in the actor tab of the toolbox.
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Class)]
    public class ActorToolboxAttribute : Attribute
    {
        /// <summary>
        /// The path to be used in the tool box
        /// </summary>
        public string Group;

        /// <summary>
        /// The name to be used for the actor in the tool box. Will default to actor name if now used.
        /// </summary>
        public string Name;

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorToolboxAttribute"/> class.
        /// </summary>
        /// <param name="group">The group to use to create the tab.</param>
        public ActorToolboxAttribute(string group)
        {
            Group = group;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorToolboxAttribute"/> class.
        /// </summary>
        /// <param name="group">The group used to create the tab.</param>
        /// <param name="name">The name to use rather than default.</param>
        public ActorToolboxAttribute(string group, string name)
        {
            Group = group;
            Name = name;
        }
    }
}
