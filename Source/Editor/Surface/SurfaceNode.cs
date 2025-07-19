// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Text;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Surface.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// The surface breakpoint data for debugger.
    /// </summary>
    [HideInEditor]
    public struct SurfaceBreakpoint
    {
        /// <summary>
        /// True if breakpoint is active, otherwise it's not set.
        /// </summary>
        public bool Set;

        /// <summary>
        /// True if breakpoint is enabled, otherwise false.
        /// </summary>
        public bool Enabled;

        /// <summary>
        /// True if breakpoint is currently hit.
        /// </summary>
        public bool Hit;
    }

    /// <summary>
    /// Visject Surface node control.
    /// </summary>
    /// <seealso cref="SurfaceControl" />
    [HideInEditor]
    public class SurfaceNode : SurfaceControl
    {
        /// <summary>
        /// The box to draw a highlight around. Drawing will be skipped if null.
        /// </summary>
        internal Box highlightBox;

        /// <summary>
        /// Flag used to discard node values setting during event sending for node UI flushing.
        /// </summary>
        protected bool _isDuringValuesEditing;

        /// <summary>
        /// The header rectangle (local space).
        /// </summary>
        protected Rectangle _headerRect;

        /// <summary>
        /// The close button rectangle (local space).
        /// </summary>
        protected Rectangle _closeButtonRect;

        /// <summary>
        /// The footer rectangle (local space).
        /// </summary>
        protected Rectangle _footerRect;

        /// <summary>
        /// The node archetype.
        /// </summary>
        public NodeArchetype Archetype;

        /// <summary>
        /// The group archetype.
        /// </summary>
        public readonly GroupArchetype GroupArchetype;

        /// <summary>
        /// The elements collection.
        /// </summary>
        public readonly List<ISurfaceNodeElement> Elements = new List<ISurfaceNodeElement>();

        /// <summary>
        /// The values (node parameters in layout based on <see cref="NodeArchetype.DefaultValues"/>).
        /// </summary>
        public object[] Values;

        /// <summary>
        /// Gets or sets the node title text.
        /// </summary>
        public string Title { get; set; }

        /// <summary>
        /// The identifier of the node.
        /// </summary>
        public readonly uint ID;

        /// <summary>
        /// Gets the type (packed GroupID (higher 16 bits) and TypeID (lower 16 bits)).
        /// </summary>
        public uint Type => ((uint)GroupArchetype.GroupID << 16) | Archetype.TypeID;

        /// <summary>
        /// The metadata.
        /// </summary>
        public readonly SurfaceMeta Meta = new SurfaceMeta();

        /// <summary>
        /// Occurs when node values collection gets changed.
        /// </summary>
        public event Action ValuesChanged;

        /// <summary>
        /// The breakpoint on the node.
        /// </summary>
        public SurfaceBreakpoint Breakpoint = new SurfaceBreakpoint();

        /// <summary>
        /// Initializes a new instance of the <see cref="SurfaceNode"/> class.
        /// </summary>
        /// <param name="id">The node id.</param>
        /// <param name="context">The surface context.</param>
        /// <param name="nodeArch">The node archetype.</param>
        /// <param name="groupArch">The group archetype.</param>
        public SurfaceNode(uint id, VisjectSurfaceContext context, NodeArchetype nodeArch, GroupArchetype groupArch)
        : base(context, nodeArch.Size.X + Constants.NodeMarginX * 2, nodeArch.Size.Y + Constants.NodeMarginY * 2 + Constants.NodeHeaderSize + Constants.NodeFooterSize)
        {
            Title = nodeArch.Title;
            ID = id;
            Archetype = nodeArch;
            GroupArchetype = groupArch;
            AutoFocus = false;
            TooltipText = GetTooltip();
            CullChildren = false;
            BackgroundColor = Style.Current.BackgroundNormal;

            if (Archetype.DefaultValues != null)
            {
                Values = new object[Archetype.DefaultValues.Length];
                Array.Copy(Archetype.DefaultValues, Values, Values.Length);
            }
        }

        /// <summary>
        /// Gets the custom text for Content Search. Can be used to include node in search results for a specific text query.
        /// </summary>
        public virtual string ContentSearchText => null;

        /// <summary>
        /// Gets the color of the footer of the node.
        /// </summary>
        protected virtual Color FooterColor => GroupArchetype.Color;

        /// <summary>
        /// Calculates the size of the node including header, footer, and margins.
        /// </summary>
        /// <param name="width">The node area width.</param>
        /// <param name="height">The node area height.</param>
        /// <returns>The node control total size.</returns>
        protected virtual Float2 CalculateNodeSize(float width, float height)
        {
            return new Float2(width + Constants.NodeMarginX * 2, height + Constants.NodeMarginY * 2 + Constants.NodeHeaderSize + Constants.NodeFooterSize);
        }

        /// <summary>
        /// Resizes the node area.
        /// </summary>
        /// <param name="width">The width.</param>
        /// <param name="height">The height.</param>
        public void Resize(float width, float height)
        {
            if (Surface == null)
                return;

            // Snap bounds (with ceil) when using grid snapping
            if (Surface.GridSnappingEnabled)
            {
                var size = Surface.SnapToGrid(new Float2(width, height), true);
                width = size.X;
                height = size.Y;
            }

            // Arrange output boxes on the right edge
            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is OutputBox box)
                {
                    box.Location = box.Archetype.Position + new Float2(width, 0);
                }
            }

            // Resize
            Size = CalculateNodeSize(width, height);
        }

        /// <summary>
        /// Automatically resizes the node to match the title size and all the elements for best fit of the node dimensions.
        /// </summary>
        public virtual void ResizeAuto()
        {
            if (Surface == null)
                return;
            var width = 0.0f;
            var height = 0.0f;
            var leftHeight = 0.0f;
            var rightHeight = 0.0f;
            var leftWidth = 40.0f;
            var rightWidth = 40.0f;
            var boxLabelFont = Style.Current.FontSmall;
            var titleLabelFont = Style.Current.FontLarge;
            for (int i = 0; i < Children.Count; i++)
            {
                var child = Children[i];
                if (!child.Visible)
                    continue;
                if (child is InputBox inputBox)
                {
                    var boxWidth = boxLabelFont.MeasureText(inputBox.Text).X + 20;
                    if (inputBox.DefaultValueEditor != null)
                        boxWidth += inputBox.DefaultValueEditor.Width + 4;
                    leftWidth = Mathf.Max(leftWidth, boxWidth);
                    leftHeight = Mathf.Max(leftHeight, inputBox.Archetype.Position.Y - Constants.NodeMarginY - Constants.NodeHeaderSize + 20.0f);
                }
                else if (child is OutputBox outputBox)
                {
                    rightWidth = Mathf.Max(rightWidth, boxLabelFont.MeasureText(outputBox.Text).X + 20);
                    rightHeight = Mathf.Max(rightHeight, outputBox.Archetype.Position.Y - Constants.NodeMarginY - Constants.NodeHeaderSize + 20.0f);
                }
                else if (child is Control control)
                {
                    if (control.AnchorPreset == AnchorPresets.TopLeft)
                    {
                        width = Mathf.Max(width, control.Right + 4 - Constants.NodeMarginX);
                        height = Mathf.Max(height, control.Bottom + 4 - Constants.NodeMarginY - Constants.NodeHeaderSize);
                    }
                    else if (!_headerRect.Intersects(control.Bounds))
                    {
                        width = Mathf.Max(width, control.Width + 4);
                        height = Mathf.Max(height, control.Height + 4);
                    }
                }
            }
            width = Mathf.Max(width, leftWidth + rightWidth + 10);
            width = Mathf.Max(width, titleLabelFont.MeasureText(Title).X + 30);
            height = Mathf.Max(height, Mathf.Max(leftHeight, rightHeight));
            Resize(width, height);
        }

        /// <summary>
        /// Creates an element from the archetype and adds the element to the node.
        /// </summary>
        /// <param name="arch">The element archetype.</param>
        /// <returns>The created element. Null if the archetype is invalid.</returns>
        public ISurfaceNodeElement AddElement(NodeElementArchetype arch)
        {
            ISurfaceNodeElement element = null;
            switch (arch.Type)
            {
            case NodeElementType.Input:
                element = new InputBox(this, arch);
                break;
            case NodeElementType.Output:
                element = new OutputBox(this, arch);
                break;
            case NodeElementType.BoolValue:
                element = new BoolValue(this, arch);
                break;
            case NodeElementType.FloatValue:
                element = new FloatValue(this, arch);
                break;
            case NodeElementType.IntegerValue:
                element = new IntegerValue(this, arch);
                break;
            case NodeElementType.ColorValue:
                element = new ColorValue(this, arch);
                break;
            case NodeElementType.ComboBox:
                element = new ComboBoxElement(this, arch);
                break;
            case NodeElementType.Asset:
                element = new AssetSelect(this, arch);
                break;
            case NodeElementType.Text:
                element = new TextView(this, arch);
                break;
            case NodeElementType.TextBox:
                element = new TextBoxView(this, arch);
                break;
            case NodeElementType.SkeletonBoneIndexSelect:
                element = new SkeletonBoneIndexSelectElement(this, arch);
                break;
            case NodeElementType.BoxValue:
                element = new BoxValue(this, arch);
                break;
            case NodeElementType.EnumValue:
                element = new EnumValue(this, arch);
                break;
            case NodeElementType.SkeletonNodeNameSelect:
                element = new SkeletonNodeNameSelectElement(this, arch);
                break;
            case NodeElementType.Actor:
                element = new ActorSelect(this, arch);
                break;
            case NodeElementType.UnsignedIntegerValue:
                element = new UnsignedIntegerValue(this, arch);
                break;
            //default: throw new NotImplementedException("Unknown node element type: " + arch.Type);
            }
            if (element != null)
            {
                AddElement(element);
            }

            return element;
        }

        /// <summary>
        /// Adds the element to the node.
        /// </summary>
        /// <param name="element">The element.</param>
        public void AddElement(ISurfaceNodeElement element)
        {
            Elements.Add(element);
            if (element is Control control)
                AddChild(control);
        }

        /// <summary>
        /// Adds the box to the node.
        /// </summary>
        /// <param name="isOut">True if add output box, otherwise it will be an input.</param>
        /// <param name="id">The box Id.</param>
        /// <param name="yLevel">The y-level in the UI.</param>
        /// <param name="text">The box text.</param>
        /// <param name="type">The box type.</param>
        /// <param name="single">If true the box will be able to have just a single connection, otherwise false.</param>
        /// <param name="valueIndex">Index of the default value in the node values array.</param>
        /// <returns>The box.</returns>
        public Box AddBox(bool isOut, int id, int yLevel, string text, ScriptType type, bool single, int valueIndex = -1)
        {
            if (type == ScriptType.Null)
                type = ScriptType.Object;

            // Try to reuse box
            var box = GetBox(id);
            if ((isOut && box is InputBox) || (!isOut && box is OutputBox))
            {
                box.Dispose();
                box = null;
            }

            // Create new if missing
            if (box == null)
            {
                if (isOut)
                    box = new OutputBox(this, NodeElementArchetype.Factory.Output(yLevel, text, type, id, single));
                else
                    box = new InputBox(this, NodeElementArchetype.Factory.Input(yLevel, text, single, type, id, valueIndex));
                AddElement(box);
            }
            else
            {
                // Sync properties for exiting box
                box.Text = text;
                box.CurrentType = type;
                box.Y = Constants.NodeMarginY + Constants.NodeHeaderSize + yLevel * Constants.LayoutOffsetY;
            }

            // Update box
            box.OnConnectionsChanged();

            return box;
        }

        /// <summary>
        /// Removes the element from the node.
        /// </summary>
        /// <param name="element">The element.</param>
        /// <param name="dispose">if set to <c>true</c> dispose control after removing, otherwise false.</param>
        public void RemoveElement(ISurfaceNodeElement element, bool dispose = true)
        {
            if (element == null)
                return;
            if (element is Box box)
                box.RemoveConnections();
            Elements.Remove(element);
            if (element is Control control)
            {
                RemoveChild(control);
                if (dispose)
                    control.Dispose();
            }
        }

        /// <summary>
        /// Removes all connections from and to that node.
        /// </summary>
        public virtual void RemoveConnections()
        {
            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is Box box)
                    box.RemoveConnections();
            }

            UpdateBoxesTypes();
        }

        /// <summary>
        /// Array of nodes that are sealed to this node - sealed nodes are duplicated/copied/pasted/removed in a batch. Null if unused.
        /// </summary>
        public virtual SurfaceNode[] SealedNodes => null;

        /// <summary>
        /// Called after adding the control to the surface after paste.
        /// </summary>
        /// <param name="idsMapping">The nodes IDs mapping (original node ID to pasted node ID). Can be used to update internal node's data after paste operation from the original data.</param>
        public virtual void OnPasted(System.Collections.Generic.Dictionary<uint, uint> idsMapping)
        {
        }

        /// <summary>
        /// Gets a value indicating whether this node uses dependent boxes.
        /// </summary>
        public bool HasDependentBoxes => Archetype.DependentBoxes != null;

        /// <summary>
        /// Gets a value indicating whether this node uses independent boxes.
        /// </summary>
        public bool HasIndependentBoxes => Archetype.IndependentBoxes != null;

        /// <summary>
        /// Gets a value indicating whether this node has dependent boxes with assigned valid types. Otherwise any box has no dependent type assigned.
        /// </summary>
        public bool HasDependentBoxesSetup
        {
            get
            {
                if (Archetype.DependentBoxes == null || Archetype.IndependentBoxes == null)
                    return true;

                for (int i = 0; i < Archetype.DependentBoxes.Length; i++)
                {
                    var b = GetBox(Archetype.DependentBoxes[i]);
                    if (b != null && b.CurrentType == b.DefaultType)
                        return false;
                }

                return true;
            }
        }

        private static readonly List<SurfaceNode> UpdateStack = new List<SurfaceNode>();

        /// <summary>
        /// Updates dependant/independent boxes types.
        /// </summary>
        public void UpdateBoxesTypes()
        {
            // Check there is no need to use box types dependency feature
            if (Archetype.DependentBoxes == null && Archetype.IndependentBoxes == null)
            {
                // Check if need to update update current type of the typeless boxes that use connection hints
                if ((Archetype.ConnectionsHints & ConnectionsHint.All) != 0)
                {
                    for (int j = 0; j < Elements.Count; j++)
                    {
                        if (Elements[j] is Box box && box.DefaultType == ScriptType.Null)
                        {
                            box.CurrentType = box.HasAnyConnection ? box.Connections[0].CurrentType : ScriptType.Null;
                        }
                    }
                }
                return;
            }

            // Prevent recursive loop call that might happen
            if (UpdateStack.Contains(this))
                return;
            UpdateStack.Add(this);

            var independentBoxesLength = Archetype.IndependentBoxes?.Length;
            var dependentBoxesLength = Archetype.DependentBoxes?.Length;

            // Get type to assign to all dependent boxes
            var type = Archetype.DefaultType;
            for (int i = 0; i < independentBoxesLength; i++)
            {
                var b = GetBox(Archetype.IndependentBoxes[i]);
                if (b != null && b.HasAnyConnection)
                {
                    // Check if the current type is set based on connection hints (eg. null box got vector connection)
                    if (type == ScriptType.Null && b.Connections[0].CurrentType != ScriptType.Null)
                    {
                        type = b.Connections[0].CurrentType;
                        break;
                    }

                    // Check if that type if part of default type
                    if (Surface != null)
                    {
                        if (Surface.CanUseDirectCast(type, b.Connections[0].DefaultType))
                        {
                            type = b.Connections[0].CurrentType;
                            break;
                        }
                    }
                    else
                    {
                        if (VisjectSurface.CanUseDirectCastStatic(type, b.Connections[0].DefaultType))
                        {
                            type = b.Connections[0].CurrentType;
                            break;
                        }
                    }
                }
            }

            // Assign connection type
            for (int i = 0; i < dependentBoxesLength; i++)
            {
                var b = GetBox(Archetype.DependentBoxes[i]);
                if (b != null)
                {
                    b.CurrentType = Archetype.DependentBoxFilter != null ? Archetype.DependentBoxFilter(b, type) : type;
                }
            }

            // Validate minor independent boxes to fit main one
            for (int i = 0; i < independentBoxesLength; i++)
            {
                var b = GetBox(Archetype.IndependentBoxes[i]);
                if (b != null)
                {
                    b.CurrentType = type;
                }
            }

            UpdateStack.Remove(this);
        }

        /// <summary>
        /// Tries to get box with given ID.
        /// </summary>
        /// <param name="id">The box id.</param>
        /// <returns>Box or null if cannot find.</returns>
        public Box GetBox(int id)
        {
            // TODO: maybe create local cache for boxes? but not a dictionary, use lookup table because ids are usually small (less than 20)
            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is Box box && box.ID == id)
                    return box;
            }
            return null;
        }

        /// <summary>
        /// Tries to get box with given ID.
        /// </summary>
        /// <param name="id">The box id.</param>
        /// <param name="result">Box or null if cannot find.</param>
        /// <returns>True fi box has been found, otherwise false.</returns>
        public bool TryGetBox(int id, out Box result)
        {
            // TODO: maybe create local cache for boxes? but not a dictionary, use lookup table because ids are usually small (less than 20)
            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is Box box && box.ID == id)
                {
                    result = box;
                    return true;
                }
            }

            result = null;
            return false;
        }

        internal List<Box> GetBoxes()
        {
            var result = new List<Box>();
            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is Box box)
                    result.Add(box);
            }
            return result;
        }

        internal void GetBoxes(List<Box> result)
        {
            result.Clear();
            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is Box box)
                    result.Add(box);
            }
        }

        internal Box GetNextBox(Box box)
        {
            // Get the one after it
            for (int i = box.IndexInParent + 1; i < Elements.Count; i++)
            {
                if (Elements[i] is Box b)
                {
                    return b;
                }
            }

            return null;
        }

        internal Box GetPreviousBox(Box box)
        {
            // Get the one before it
            for (int i = box.IndexInParent - 1; i >= 0; i--)
            {
                if (Elements[i] is Box b)
                {
                    return b;
                }
            }

            return null;
        }

        /// <summary>
        /// Returns true if any box is selected by the user (one or more).
        /// </summary>
        public bool HasBoxesSelection
        {
            get
            {
                if (!IsSelected)
                    return false;
                for (int i = 0; i < Elements.Count; i++)
                {
                    if (Elements[i] is Box box && box.IsSelected)
                        return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Selects all the boxes.
        /// </summary>
        public void SelectAllBoxes()
        {
            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is Box box)
                    box.IsSelected = true;
            }
        }

        /// <summary>
        /// Clears the box selection.
        /// </summary>
        public void ClearBoxSelection()
        {
            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is Box box)
                    box.IsSelected = false;
            }
        }

        /// <summary>
        /// Adds the specified box to the selection.
        /// </summary>
        /// <param name="box">The box.</param>
        public void AddBoxToSelection(Box box)
        {
            box.IsSelected = true;
        }

        /// <summary>
        /// Selects the specified control.
        /// </summary>
        /// <param name="box">The box.</param>
        public void SelectBox(Box box)
        {
            ClearBoxSelection();

            box.IsSelected = true;
        }

        /// <summary>
        /// Selects the specified controls collection.
        /// </summary>
        /// <param name="boxes">The boxes.</param>
        public void SelectBoxes(IEnumerable<Box> boxes)
        {
            ClearBoxSelection();

            foreach (var box in boxes)
            {
                box.IsSelected = true;
            }
        }

        /// <summary>
        /// Deselects the specified control.
        /// </summary>
        /// <param name="box">The box.</param>
        public void DeselectBox(Box box)
        {
            box.IsSelected = false;
        }

        /// <summary>
        /// Called when node gets selected or deselected.
        /// </summary>
        protected override void OnSelectionChanged()
        {
            if (!IsSelected)
            {
                ClearBoxSelection();
            }
        }

        /// <summary>
        /// Implementation of Depth-First traversal over the graph of surface nodes.
        /// </summary>
        /// <param name="traverseInputs">True if perform traversal over node inputs. Otherwise will use output boxes.</param>
        /// <param name="throwOnCycle">True if throw exception on recursive cycle detection.</param>
        /// <returns>The list of nodes as a result of depth-first traversal algorithm execution.</returns>
        public IEnumerable<SurfaceNode> DepthFirstTraversal(bool traverseInputs, bool throwOnCycle = false)
        {
            // Reference: https://github.com/stefnotch/flax-custom-visject-plugin/blob/a26a98b40f909a0b10c2259b858e058290003dce/Source/Editor/ExpressionGraphSurface.cs#L231

            // The states of a node are 
            // null  Nothing   (unvisited and not on the stack)
            // false Processing(  visited and     on the stack)
            // true  Completed (  visited and not on the stack)
            var nodeState = new Dictionary<SurfaceNode, bool>();
            var toProcess = new Stack<SurfaceNode>();
            var output = new List<SurfaceNode>();

            // Start processing the nodes (backwards)
            toProcess.Push(this);
            while (toProcess.Count > 0)
            {
                var node = toProcess.Peek();

                // We have never seen this node before
                if (!nodeState.ContainsKey(node))
                {
                    // We are now processing it
                    nodeState.Add(node, false);
                }
                else
                {
                    // Otherwise, we are done processing it
                    nodeState[node] = true;

                    // Remove it from the stack
                    toProcess.Pop();

                    // And add it to the output
                    output.Add(node);
                }

                // For all parents, push them onto the stack if they haven't been visited yet
                var elements = node.Elements;
                for (int i = 0; i < elements.Count; i++)
                {
                    var box = node.Elements[i] as Box;
                    if ((traverseInputs && box is InputBox) || (!traverseInputs && box is OutputBox))
                    {
                        foreach (var connection in box.Connections)
                        {
                            // It has been visited previously
                            if (nodeState.TryGetValue(connection.ParentNode, out bool state))
                            {
                                if (state == false && throwOnCycle)
                                {
                                    // It's still processing, so there must be a cycle!
                                    throw new Exception("Cycle detected!");
                                }
                            }
                            else
                            {
                                // It hasn't been visited, add it to the stack
                                toProcess.Push(connection.ParentNode);
                            }
                        }
                    }
                }
            }

            return output;
        }

        /// <summary>
        /// Draws all the connections between surface objects related to this node.
        /// </summary>
        /// <param name="mousePosition">The current mouse position (in surface-space).</param>
        public virtual void DrawConnections(ref Float2 mousePosition)
        {
            for (int j = 0; j < Elements.Count; j++)
            {
                if (Elements[j] is OutputBox ob && ob.HasAnyConnection)
                {
                    ob.DrawConnections(ref mousePosition);
                }
            }
        }

        /// <summary>
        /// Draws all selected connections between surface objects related to this node.
        /// </summary>
        /// <param name="selectedConnectionIndex">The index of the currently selected connection.</param>
        public void DrawSelectedConnections(int selectedConnectionIndex)
        {
            if (_isSelected)
            {
                if (HasBoxesSelection)
                {
                    for (int j = 0; j < Elements.Count; j++)
                    {
                        if (Elements[j] is Box box && box.IsSelected && selectedConnectionIndex < box.Connections.Count)
                        {
                            if (box is OutputBox ob)
                            {
                                ob.DrawSelectedConnection(ob.Connections[selectedConnectionIndex]);
                            }
                            else
                            {
                                if (box.Connections[selectedConnectionIndex] is OutputBox outputBox)
                                {
                                    outputBox.DrawSelectedConnection(box);
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (int j = 0; j < Elements.Count; j++)
                    {
                        if (Elements[j] is Box box)
                        {
                            if (box is OutputBox ob)
                            {
                                for (int i = 0; i < ob.Connections.Count; i++)
                                {
                                    ob.DrawSelectedConnection(ob.Connections[i]);
                                }
                            }
                            else
                            {
                                for (int i = 0; i < box.Connections.Count; i++)
                                {
                                    if (box.Connections[i] is OutputBox outputBox)
                                    {
                                        outputBox.DrawSelectedConnection(box);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Custom function to check if node matches a given search query.
        /// </summary>
        /// <param name="text">Text to check.</param>
        /// <returns>True if node contains a given value.</returns>
        public virtual bool Search(string text)
        {
            return false;
        }

        private string GetTooltip()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(string.IsNullOrEmpty(Archetype.Signature) ? Archetype.Title : Archetype.Signature);
            if (!string.IsNullOrEmpty(Archetype.Description))
                sb.Append("\n" + Archetype.Description);
            return sb.ToString();
        }

        /// <inheritdoc />
        protected override bool ShowTooltip => base.ShowTooltip && _headerRect.Contains(ref _mousePosition) && !Surface.IsLeftMouseButtonDown && !Surface.IsRightMouseButtonDown && !Surface.IsPrimaryMenuOpened;

        /// <inheritdoc />
        public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
        {
            var result = base.OnShowTooltip(out text, out _, out area);

            // Change the position
            location = new Float2(_headerRect.Width * 0.5f, _headerRect.Bottom);

            return result;
        }

        /// <inheritdoc />
        public override void OnSurfaceCanEditChanged(bool canEdit)
        {
            base.OnSurfaceCanEditChanged(canEdit);

            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is SurfaceNodeElementControl element)
                    element.OnSurfaceCanEditChanged(canEdit);
                else if (Elements[i] is Control control)
                    control.Enabled = canEdit;
            }
        }

        /// <inheritdoc />
        public override bool OnTestTooltipOverControl(ref Float2 location)
        {
            return _headerRect.Contains(ref location) && ShowTooltip && !Surface.IsConnecting && !Surface.IsBoxSelecting;
        }

        /// <inheritdoc />
        public override bool CanSelect(ref Float2 location)
        {
            return _headerRect.MakeOffsetted(Location).Contains(ref location);
        }

        /// <inheritdoc />
        public override void OnSurfaceLoaded(SurfaceNodeActions action)
        {
            base.OnSurfaceLoaded(action);

            UpdateBoxesTypes();

            for (int i = 0; i < Elements.Count; i++)
            {
                if (Elements[i] is Box box)
                    box.OnConnectionsChanged();
            }
        }

        /// <inheritdoc />
        public override void OnDeleted(SurfaceNodeActions action)
        {
            RemoveConnections();

            base.OnDeleted(action);
        }

        /// <summary>
        /// Sets the value of the node parameter.
        /// </summary>
        /// <param name="index">The value index.</param>
        /// <param name="value">The value.</param>
        /// <param name="graphEdited">True if graph has been edited (nodes structure or parameter value).</param>
        public virtual void SetValue(int index, object value, bool graphEdited = true)
        {
            if (_isDuringValuesEditing || (Surface != null && !Surface.CanEdit))
                return;
            if (FlaxEngine.Json.JsonSerializer.ValueEquals(value, Values[index]))
                return;
            if (value is byte[] && Values[index] is byte[] && Utils.ArraysEqual((byte[])value, (byte[])Values[index]))
                return;

            _isDuringValuesEditing = true;

            var before = Surface?.Undo != null ? (object[])Values.Clone() : null;

            Values[index] = value;
            OnValuesChanged();
            Surface?.MarkAsEdited(graphEdited);

            if (Surface != null)
                Surface.AddBatchedUndoAction(new EditNodeValuesAction(this, before, graphEdited));

            _isDuringValuesEditing = false;
        }

        /// <summary>
        /// Sets the values of the node parameters.
        /// </summary>
        /// <param name="values">The values.</param>
        /// <param name="graphEdited">True if graph has been edited (nodes structure or parameter value).</param>
        public virtual void SetValues(object[] values, bool graphEdited = true)
        {
            if (_isDuringValuesEditing || !Surface.CanEdit)
                return;
            if (values == null || Values == null)
                throw new ArgumentException();
            bool resize = values.Length != Values.Length;
            if (resize && (Archetype.Flags & NodeFlags.VariableValuesSize) == 0)
                throw new ArgumentException();

            _isDuringValuesEditing = true;

            var before = Surface.Undo != null ? (object[])Values.Clone() : null;

            if (resize)
                Values = (object[])values.Clone();
            else
                Array.Copy(values, Values, values.Length);
            OnValuesChanged();

            if (Surface != null)
            {
                Surface.MarkAsEdited(graphEdited);
                Surface.AddBatchedUndoAction(new EditNodeValuesAction(this, before, graphEdited));
            }

            _isDuringValuesEditing = false;
        }

        internal void SetIsDuringValuesEditing(bool value)
        {
            _isDuringValuesEditing = value;
        }

        /// <summary>
        /// Sets teh node values from the given pasted source. Can be overriden to perform validation or custom values processing.
        /// </summary>
        /// <param name="values">The input values array.</param>
        public virtual void SetValuesPaste(object[] values)
        {
            Values = values;
        }

        /// <summary>
        /// Called when node values set gets changed.
        /// </summary>
        public virtual void OnValuesChanged()
        {
            ValuesChanged?.Invoke();
            Surface?.OnNodeValuesEdited(this);
        }

        /// <summary>
        /// Updates the given box connection.
        /// </summary>
        /// <param name="box">The box.</param>
        public virtual void ConnectionTick(Box box)
        {
            UpdateBoxesTypes();
        }

        /// <inheritdoc />
        protected override void UpdateRectangles()
        {
            const float footerSize = Constants.NodeFooterSize;
            const float headerSize = Constants.NodeHeaderSize;
            const float closeButtonMargin = Constants.NodeCloseButtonMargin;
            const float closeButtonSize = Constants.NodeCloseButtonSize;
            _headerRect = new Rectangle(0, 0, Width, headerSize);
            _closeButtonRect = new Rectangle(Width - closeButtonSize - closeButtonMargin, closeButtonMargin, closeButtonSize, closeButtonSize);
            _footerRect = new Rectangle(0, Height - footerSize, Width, footerSize);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;

            // Background
            var backgroundRect = new Rectangle(Float2.Zero, Size);
            Render2D.FillRectangle(backgroundRect, BackgroundColor);

            // Breakpoint hit
            if (Breakpoint.Hit)
            {
                var colorTop = Color.OrangeRed;
                var colorBottom = Color.Red;
                var time = DateTime.Now - Engine.StartupTime;
                Render2D.DrawRectangle(backgroundRect.MakeExpanded(Mathf.Lerp(3.0f, 12.0f, Mathf.Sin((float)time.TotalSeconds * 10.0f) * 0.5f + 0.5f)), colorTop, colorTop, colorBottom, colorBottom, 2.0f);
            }

            // Header
            var headerColor = style.BackgroundHighlighted;
            if (_headerRect.Contains(ref _mousePosition) && !Surface.IsConnecting && !Surface.IsBoxSelecting)
                headerColor *= 1.07f;
            Render2D.FillRectangle(_headerRect, headerColor);
            Render2D.DrawText(style.FontLarge, Title, _headerRect, style.Foreground, TextAlignment.Center, TextAlignment.Center);

            // Close button
            if ((Archetype.Flags & NodeFlags.NoCloseButton) == 0 && Surface.CanEdit)
            {
                bool highlightClose = _closeButtonRect.Contains(_mousePosition) && !Surface.IsConnecting && !Surface.IsBoxSelecting;
                Render2D.DrawSprite(style.Cross, _closeButtonRect, highlightClose ? style.Foreground : style.ForegroundGrey);
            }

            // Footer
            Render2D.FillRectangle(_footerRect, FooterColor);

            DrawChildren();

            // Selection outline
            if (_isSelected)
            {
                var colorTop = Color.Orange;
                var colorBottom = Color.OrangeRed;
                Render2D.DrawRectangle(backgroundRect, colorTop, colorTop, colorBottom, colorBottom);
            }

            // Breakpoint dot
            if (Breakpoint.Set)
            {
                var icon = Breakpoint.Enabled ? Surface.Style.Icons.BoxClose : Surface.Style.Icons.BoxOpen;
                Render2D.DrawSprite(icon, new Rectangle(-7, -7, 16, 16), new Color(0.9f, 0.9f, 0.9f));
                Render2D.DrawSprite(icon, new Rectangle(-6, -6, 14, 14), new Color(0.894117647f, 0.0784313725f, 0.0f));
            }

            if (highlightBox != null)
                Render2D.DrawRectangle(highlightBox.Bounds, style.BorderHighlighted, 2f);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left && (Archetype.Flags & NodeFlags.NoCloseButton) == 0 && _closeButtonRect.Contains(ref location))
                return true;
            if (button == MouseButton.Right)
                return true;

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            // Close/ delete
            bool canDelete = !Surface.IsConnecting && !Surface.WasBoxSelecting && !Surface.WasMovingSelection;
            if (button == MouseButton.Left && canDelete && (Archetype.Flags & NodeFlags.NoCloseButton) == 0 && _closeButtonRect.Contains(ref location))
            {
                Surface.Delete(this);
                return true;
            }

            // Secondary Context Menu
            if (button == MouseButton.Right)
            {
                if (!IsSelected)
                    Surface.Select(this);
                var tmp = PointToParent(ref location);
                Surface.ShowSecondaryCM(Parent.PointToParent(ref tmp), this);
                return true;
            }

            return false;
        }
    }
}
