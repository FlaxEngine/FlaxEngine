// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Surface.Archetypes
{
    /// <summary>
    /// Contains archetypes for nodes from the Behavior Tree group.
    /// </summary>
    [HideInEditor]
    public static class BehaviorTree
    {
        /// <summary>
        /// Customized <see cref="SurfaceNode" /> for the Behavior Tree node.
        /// </summary>
        internal class Node : SurfaceNode
        {
            private const float ConnectionAreaMargin = 12.0f;
            private const float ConnectionAreaHeight = 12.0f;

            private ScriptType _type;
            private InputBox _input;
            private OutputBox _output;

            public BehaviorTreeNode Instance;

            internal static SurfaceNode Create(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            {
                return new Node(id, context, nodeArch, groupArch);
            }

            internal Node(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
            : base(id, context, nodeArch, groupArch)
            {
            }

            public static string GetTitle(ScriptType scriptType)
            {
                var title = scriptType.Name;
                if (title.StartsWith("BehaviorTree"))
                    title = title.Substring(12);
                if (title.EndsWith("Node"))
                    title = title.Substring(0, title.Length - 4);
                title = Utilities.Utils.GetPropertyNameUI(title);
                return title;
            }

            private void UpdateTitle()
            {
                string title = null;
                if (Instance != null)
                {
                    title = Instance.Name;
                    if (string.IsNullOrEmpty(title))
                        title = GetTitle(_type);
                }
                else
                {
                    var typeName = (string)Values[0];
                    title = "Missing Type " + typeName;
                }
                if (title != Title)
                {
                    Title = title;
                    ResizeAuto();
                }
            }

            /// <inheritdoc />
            public override void OnLoaded()
            {
                base.OnLoaded();

                // Setup boxes
                _input = (InputBox)GetBox(0);
                _output = (OutputBox)GetBox(1);

                // Setup node type and data
                var flagsRoot = NodeFlags.NoRemove | NodeFlags.NoCloseButton | NodeFlags.NoSpawnViaPaste;
                var flags = Archetype.Flags & ~flagsRoot;
                var typeName = (string)Values[0];
                _type = TypeUtils.GetType(typeName);
                if (_type != null)
                {
                    bool isRoot = _type.Type == typeof(BehaviorTreeRootNode);
                    _input.Enabled = _input.Visible = !isRoot;
                    _output.Enabled = _output.Visible = new ScriptType(typeof(BehaviorTreeCompoundNode)).IsAssignableFrom(_type);
                    if (isRoot)
                        flags |= flagsRoot;
                    TooltipText = Editor.Instance.CodeDocs.GetTooltip(_type);
                    try
                    {
                        // Load node instance from data
                        Instance = (BehaviorTreeNode)_type.CreateInstance();
                        var instanceData = (byte[])Values[1];
                        FlaxEngine.Json.JsonSerializer.LoadFromBytes(Instance, instanceData, Globals.EngineBuildNumber);
                    }
                    catch (Exception ex)
                    {
                        Editor.LogError("Failed to load Behavior Tree node of type " + typeName);
                        Editor.LogWarning(ex);
                    }
                }
                else
                {
                    Instance = null;
                }
                if (Archetype.Flags != flags)
                {
                    // Apply custom flags
                    Archetype = (NodeArchetype)Archetype.Clone();
                    Archetype.Flags = flags;
                }

                UpdateTitle();
            }

            /// <inheritdoc />
            public override void OnSpawned()
            {
                base.OnSpawned();

                ResizeAuto();
            }

            /// <inheritdoc />
            public override void ResizeAuto()
            {
                if (Surface == null)
                    return;
                var width = 0.0f;
                var height = 0.0f;
                var titleLabelFont = Style.Current.FontLarge;
                width = Mathf.Max(width, 100.0f);
                width = Mathf.Max(width, titleLabelFont.MeasureText(Title).X + 30);
                if (_input != null && _input.Visible)
                    height += ConnectionAreaHeight;
                if (_output != null && _output.Visible)
                    height += ConnectionAreaHeight;
                Size = new Float2(width + FlaxEditor.Surface.Constants.NodeMarginX * 2, height + FlaxEditor.Surface.Constants.NodeHeaderSize + FlaxEditor.Surface.Constants.NodeFooterSize);
                if (_input != null && _input.Visible)
                    _input.Bounds = new Rectangle(ConnectionAreaMargin, 0, Width - ConnectionAreaMargin * 2, ConnectionAreaHeight);
                if (_output != null && _output.Visible)
                    _output.Bounds = new Rectangle(ConnectionAreaMargin, Height - ConnectionAreaHeight, Width - ConnectionAreaMargin * 2, ConnectionAreaHeight);
            }

            /// <inheritdoc />
            protected override void UpdateRectangles()
            {
                base.UpdateRectangles();

                // Update boxes placement
                const float footerSize = FlaxEditor.Surface.Constants.NodeFooterSize;
                const float headerSize = FlaxEditor.Surface.Constants.NodeHeaderSize;
                const float closeButtonMargin = FlaxEditor.Surface.Constants.NodeCloseButtonMargin;
                const float closeButtonSize = FlaxEditor.Surface.Constants.NodeCloseButtonSize;
                _headerRect = new Rectangle(0, 0, Width, headerSize);
                if (_input != null && _input.Visible)
                    _headerRect.Y += ConnectionAreaHeight;
                _footerRect = new Rectangle(0, _headerRect.Bottom, Width, footerSize);
                _closeButtonRect = new Rectangle(Width - closeButtonSize - closeButtonMargin, _headerRect.Y + closeButtonMargin, closeButtonSize, closeButtonSize);
            }

            /// <inheritdoc />
            public override void OnValuesChanged()
            {
                base.OnValuesChanged();

                try
                {
                    if (Instance != null)
                    {
                        // Reload node instance from data
                        var instanceData = (byte[])Values[1];
                        if (instanceData == null || instanceData.Length == 0)
                        {
                            // Recreate instance data to default state if previous state was empty
                            var defaultInstance = (BehaviorTreeNode)_type.CreateInstance(); // TODO: use default instance from native ScriptingType
                            instanceData = FlaxEngine.Json.JsonSerializer.SaveToBytes(defaultInstance);
                        }
                        FlaxEngine.Json.JsonSerializer.LoadFromBytes(Instance, instanceData, Globals.EngineBuildNumber);
                        UpdateTitle();
                    }
                }
                catch (Exception ex)
                {
                    Editor.LogError("Failed to load Behavior Tree node of type " + _type);
                    Editor.LogWarning(ex);
                }
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                if (IsDisposing)
                    return;
                _type = ScriptType.Null;
                Object.Destroy(ref Instance);

                base.OnDestroy();
            }
        }

        /// <summary>
        /// The nodes for that group.
        /// </summary>
        public static NodeArchetype[] Nodes =
        {
            new NodeArchetype
            {
                TypeID = 1,
                Create = Node.Create,
                Flags = NodeFlags.BehaviorTreeGraph | NodeFlags.NoSpawnViaGUI,
                DefaultValues = new object[]
                {
                    string.Empty, // Type Name
                    Utils.GetEmptyArray<byte>(), // Instance Data
                },
                Size = new Float2(100, 0),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, ScriptType.Void, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, ScriptType.Void, 1),
                }
            },
            new NodeArchetype
            {
                TypeID = 2,
                Create = Node.Create,
                Flags = NodeFlags.BehaviorTreeGraph | NodeFlags.NoSpawnViaGUI,
                DefaultValues = new object[]
                {
                    typeof(BehaviorTreeRootNode).FullName, // Root node
                    Utils.GetEmptyArray<byte>(), // Instance Data
                },
                Size = new Float2(100, 0),
                Elements = new[]
                {
                    NodeElementArchetype.Factory.Input(0, string.Empty, true, ScriptType.Void, 0),
                    NodeElementArchetype.Factory.Output(0, string.Empty, ScriptType.Void, 1),
                }
            },
        };
    }
}
