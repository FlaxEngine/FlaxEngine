// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEngine;
using FlaxEngine.Assertions;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// It's responsible for creating new <see cref="SurfaceNode"/> objects.
    /// It contains collection of <see cref="GroupArchetype"/> and allows to plug custom node types as well.
    /// </summary>
    [HideInEditor]
    public static class NodeFactory
    {
        /// <summary>
        /// The list of supported attribute types for Visject Surface parameters. Attributes must be marked with Serializable attribute and contain empty constructor.
        /// </summary>
        public static readonly List<Type> ParameterAttributeTypes = new List<Type>(16)
        {
            typeof(TooltipAttribute),
            typeof(LimitAttribute),
            typeof(EditorDisplayAttribute),
            typeof(EditorOrderAttribute),
            typeof(HeaderAttribute),
            typeof(RangeAttribute),
            typeof(SpaceAttribute),
            typeof(NoAnimateAttribute),
            typeof(NoUndoAttribute),
        };

        /// <summary>
        /// The list of supported attribute types for Visject Surface function nodes. Attributes must be marked with Serializable attribute and contain empty constructor.
        /// </summary>
        public static readonly List<Type> FunctionAttributeTypes = new List<Type>(8)
        {
            typeof(TooltipAttribute),
            typeof(HideInEditorAttribute),
            typeof(NoAnimateAttribute),
            typeof(ButtonAttribute),
        };

        /// <summary>
        /// The list of supported attribute types for Visject Surface type. Attributes must be marked with Serializable attribute and contain empty constructor.
        /// </summary>
        public static readonly List<Type> TypeAttributeTypes = new List<Type>(8)
        {
            typeof(TooltipAttribute),
            typeof(HideInEditorAttribute),
        };

        /// <summary>
        /// The default Visject Node archetype groups collection.
        /// </summary>
        public static readonly List<GroupArchetype> DefaultGroups = new List<GroupArchetype>(32)
        {
            new GroupArchetype
            {
                GroupID = 1,
                Name = "Material",
                Color = new Color(231, 76, 60),
                Archetypes = Archetypes.Material.Nodes
            },
            new GroupArchetype
            {
                GroupID = 2,
                Name = "Constants",
                Color = new Color(243, 156, 18),
                Archetypes = Archetypes.Constants.Nodes
            },
            new GroupArchetype
            {
                GroupID = 3,
                Name = "Math",
                Color = new Color(52, 152, 219),
                Archetypes = Archetypes.Math.Nodes
            },
            new GroupArchetype
            {
                GroupID = 4,
                Name = "Packing",
                Color = new Color(155, 89, 182),
                Archetypes = Archetypes.Packing.Nodes
            },
            new GroupArchetype
            {
                GroupID = 5,
                Name = "Textures",
                Color = new Color(46, 204, 113),
                Archetypes = Archetypes.Textures.Nodes
            },
            new GroupArchetype
            {
                GroupID = 6,
                Name = "Parameters",
                Color = new Color(52, 73, 94),
                Archetypes = Archetypes.Parameters.Nodes
            },
            new GroupArchetype
            {
                GroupID = 7,
                Name = "Tools",
                Color = new Color(149, 165, 166),
                Archetypes = Archetypes.Tools.Nodes
            },
            new GroupArchetype
            {
                GroupID = 8,
                Name = "Layers",
                Color = new Color(249, 105, 116),
                Archetypes = Archetypes.Layers.Nodes
            },
            new GroupArchetype
            {
                GroupID = 9,
                Name = "Animations",
                Color = new Color(105, 179, 160),
                Archetypes = Archetypes.Animation.Nodes
            },
            new GroupArchetype
            {
                GroupID = 10,
                Name = "Boolean",
                Color = new Color(237, 28, 36),
                Archetypes = Archetypes.Boolean.Nodes
            },
            new GroupArchetype
            {
                GroupID = 11,
                Name = "Bitwise",
                Color = new Color(181, 230, 29),
                Archetypes = Archetypes.Bitwise.Nodes
            },
            new GroupArchetype
            {
                GroupID = 12,
                Name = "Comparisons",
                Color = new Color(148, 30, 34),
                Archetypes = Archetypes.Comparisons.Nodes
            },
            // GroupID = 13 -> Custom Nodes provided externally
            new GroupArchetype
            {
                GroupID = 14,
                Name = "Particles",
                Color = new Color(121, 210, 176),
                Archetypes = Archetypes.Particles.Nodes
            },
            new GroupArchetype
            {
                GroupID = 15,
                Name = "Particle Modules",
                Color = new Color(221, 110, 176),
                Archetypes = Archetypes.ParticleModules.Nodes
            },
            new GroupArchetype
            {
                GroupID = 16,
                Name = "Function",
                Color = new Color(109, 160, 24),
                Archetypes = Archetypes.Function.Nodes
            },
            new GroupArchetype
            {
                GroupID = 17,
                Name = "Flow",
                Color = new Color(237, 136, 64),
                Archetypes = Archetypes.Flow.Nodes
            },
            new GroupArchetype
            {
                GroupID = 18,
                Name = "Collections",
                Color = new Color(110, 180, 81),
                Archetypes = Archetypes.Collections.Nodes
            },
            new GroupArchetype
            {
                GroupID = 19,
                Name = "Behavior Tree",
                Color = new Color(70, 220, 181),
                Archetypes = Archetypes.BehaviorTree.Nodes
            },
        };

        /// <summary>
        /// Gets the archetypes for the node.
        /// </summary>
        /// <param name="groups">The group archetypes.</param>
        /// <param name="groupID">The group identifier.</param>
        /// <param name="typeID">The type identifier.</param>
        /// <param name="gArch">The output group archetype.</param>
        /// <param name="arch">The output node archetype.</param>
        /// <returns>True if found it, otherwise false.</returns>
        public static bool GetArchetype(List<GroupArchetype> groups, ushort groupID, ushort typeID, out GroupArchetype gArch, out NodeArchetype arch)
        {
            gArch = null;
            arch = null;

            // Find archetype for that node
            foreach (var groupArchetype in groups)
            {
                if (groupArchetype.GroupID == groupID && groupArchetype.Archetypes != null)
                {
                    foreach (var nodeArchetype in groupArchetype.Archetypes)
                    {
                        if (nodeArchetype.TypeID == typeID)
                        {
                            // Found
                            gArch = groupArchetype;
                            arch = nodeArchetype;
                            return true;
                        }
                    }
                }
            }

            // Error
            Editor.LogError($"Failed to create Visject Surface node with id: {groupID}:{typeID}");
            return false;
        }

        /// <summary>
        /// Creates the node.
        /// </summary>
        /// <param name="groups">The group archetypes.</param>
        /// <param name="id">The node id.</param>
        /// <param name="context">The context.</param>
        /// <param name="groupID">The group identifier.</param>
        /// <param name="typeID">The type identifier.</param>
        /// <returns>Created node or null if failed.</returns>
        public static SurfaceNode CreateNode(List<GroupArchetype> groups, uint id, VisjectSurfaceContext context, ushort groupID, ushort typeID)
        {
            for (var i = 0; i < groups.Count; i++)
            {
                var groupArchetype = groups[i];
                if (groupArchetype.GroupID == groupID && groupArchetype.Archetypes != null)
                {
                    foreach (var nodeArchetype in groupArchetype.Archetypes)
                    {
                        if (nodeArchetype.TypeID == typeID)
                        {
                            SurfaceNode node;
                            if (nodeArchetype.Create != null)
                                node = nodeArchetype.Create(id, context, nodeArchetype, groupArchetype);
                            else
                                node = new SurfaceNode(id, context, nodeArchetype, groupArchetype);
                            return node;
                        }
                    }
                }
            }
            return null;
        }

        /// <summary>
        /// Creates the node.
        /// </summary>
        /// <param name="id">The node id.</param>
        /// <param name="context">The context.</param>
        /// <param name="groupArchetype">The group archetype.</param>
        /// <param name="nodeArchetype">The node archetype.</param>
        /// <returns>Created node or null if failed.</returns>
        public static SurfaceNode CreateNode(uint id, VisjectSurfaceContext context, GroupArchetype groupArchetype, NodeArchetype nodeArchetype)
        {
            Assert.IsTrue(groupArchetype.Archetypes.Contains(nodeArchetype));

            SurfaceNode node;
            if (nodeArchetype.Create != null)
                node = nodeArchetype.Create(id, context, nodeArchetype, groupArchetype);
            else
                node = new SurfaceNode(id, context, nodeArchetype, groupArchetype);
            return node;
        }
    }
}
