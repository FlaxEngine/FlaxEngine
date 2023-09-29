// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used when no specified inspector is provided for the type. Inspector 
    /// displays GUI for all the inspectable fields in the object.
    /// </summary>
    public class GenericEditor : CustomEditor
    {
        /// <summary>
        /// Describes object property/field information for custom editors pipeline.
        /// </summary>
        /// <seealso cref="System.IComparable" />
        protected class ItemInfo : IComparable
        {
            private Options.GeneralOptions.MembersOrder _membersOrder;

            /// <summary>
            /// The member information from reflection.
            /// </summary>
            public ScriptMemberInfo Info;

            /// <summary>
            /// The order attribute.
            /// </summary>
            public EditorOrderAttribute Order;

            /// <summary>
            /// The display attribute.
            /// </summary>
            public EditorDisplayAttribute Display;

            /// <summary>
            /// The custom editor attribute.
            /// </summary>
            public CustomEditorAttribute CustomEditor;

            /// <summary>
            /// The custom editor alias attribute.
            /// </summary>
            public CustomEditorAliasAttribute CustomEditorAlias;

            /// <summary>
            /// The space attribute.
            /// </summary>
            public SpaceAttribute Space;

            /// <summary>
            /// The header attribute.
            /// </summary>
            public HeaderAttribute Header;

            /// <summary>
            /// The visible if attribute.
            /// </summary>
            public VisibleIfAttribute VisibleIf;

            /// <summary>
            /// The "visible if value" attribute.
            /// </summary>
            public VisibleIfValueAttribute VisibleIfValue;

            /// <summary>
            /// The read-only attribute usage flag.
            /// </summary>
            public bool IsReadOnly;

            /// <summary>
            /// The expand groups flag.
            /// </summary>
            public bool ExpandGroups;

            /// <summary>
            /// Gets the display name.
            /// </summary>
            public string DisplayName { get; }

            /// <summary>
            /// Gets the overridden custom editor for item editing.
            /// </summary>
            public CustomEditor OverrideEditor
            {
                get
                {
                    if (CustomEditor != null)
                        return (CustomEditor)Activator.CreateInstance(CustomEditor.Type);
                    if (CustomEditorAlias != null)
                        return (CustomEditor)TypeUtils.CreateInstance(CustomEditorAlias.TypeName);
                    return null;
                }
            }

            /// <summary>
            /// Gets the tooltip text (may be null if not provided).
            /// </summary>
            public string TooltipText;

            /// <summary>
            /// Initializes a new instance of the <see cref="ItemInfo"/> class.
            /// </summary>
            /// <param name="info">The reflection information.</param>
            public ItemInfo(ScriptMemberInfo info)
            : this(info, info.GetAttributes(true))
            {
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="ItemInfo"/> class.
            /// </summary>
            /// <param name="info">The reflection information.</param>
            /// <param name="attributes">The attributes.</param>
            public ItemInfo(ScriptMemberInfo info, object[] attributes)
            {
                Info = info;
                Order = (EditorOrderAttribute)attributes.FirstOrDefault(x => x is EditorOrderAttribute);
                Display = (EditorDisplayAttribute)attributes.FirstOrDefault(x => x is EditorDisplayAttribute);
                CustomEditor = (CustomEditorAttribute)attributes.FirstOrDefault(x => x is CustomEditorAttribute);
                CustomEditorAlias = (CustomEditorAliasAttribute)attributes.FirstOrDefault(x => x is CustomEditorAliasAttribute);
                Space = (SpaceAttribute)attributes.FirstOrDefault(x => x is SpaceAttribute);
                Header = (HeaderAttribute)attributes.FirstOrDefault(x => x is HeaderAttribute);
                VisibleIf = (VisibleIfAttribute)attributes.FirstOrDefault(x => x is VisibleIfAttribute);
                VisibleIfValue = (VisibleIfValueAttribute)attributes.FirstOrDefault(x => x is VisibleIfValueAttribute);
                IsReadOnly = attributes.FirstOrDefault(x => x is ReadOnlyAttribute) != null;
                ExpandGroups = attributes.FirstOrDefault(x => x is ExpandGroupsAttribute) != null;

                IsReadOnly |= !info.HasSet;
                DisplayName = Display?.Name ?? Utilities.Utils.GetPropertyNameUI(info.Name);
                var editor = Editor.Instance;
                TooltipText = editor.CodeDocs.GetTooltip(info, attributes);
                _membersOrder = editor.Options.Options.General.ScriptMembersOrder;
            }

            /// <summary>
            /// Gets the values.
            /// </summary>
            /// <param name="instanceValues">The instance values.</param>
            /// <returns>The values container.</returns>
            public ValueContainer GetValues(ValueContainer instanceValues)
            {
                return new ValueContainer(Info, instanceValues);
            }

            /// <inheritdoc />
            public int CompareTo(object obj)
            {
                if (obj is ItemInfo other)
                {
                    // By order
                    if (Order != null)
                    {
                        if (other.Order != null)
                            return Order.Order - other.Order.Order;
                        return -1;
                    }
                    if (other.Order != null)
                        return 1;

                    // By group name
                    if (Display?.Group != null)
                    {
                        if (other.Display?.Group != null)
                            return string.Compare(Display.Group, other.Display.Group, StringComparison.InvariantCulture);
                    }

                    if (_membersOrder == Options.GeneralOptions.MembersOrder.Declaration)
                    {
                        // By declaration order
                        if (Info.MetadataToken > other.Info.MetadataToken)
                            return 1;
                        if (Info.MetadataToken < other.Info.MetadataToken)
                            return -1;
                    }

                    // By name
                    return string.Compare(Info.Name, other.Info.Name, StringComparison.InvariantCulture);
                }

                return 0;
            }

            /// <inheritdoc />
            public override string ToString()
            {
                return Info.Name;
            }

            /// <summary>
            /// Determines whether can merge two item infos to present them at once.
            /// </summary>
            /// <param name="a">The a.</param>
            /// <param name="b">The b.</param>
            /// <returns><c>true</c> if can merge two item infos to present them at once; otherwise, <c>false</c>.</returns>
            public static bool CanMerge(ItemInfo a, ItemInfo b)
            {
                if (a.Info.DeclaringType != b.Info.DeclaringType)
                    return false;
                return a.Info.Name == b.Info.Name;
            }
        }

        private abstract class VisibleIfBaseCache
        {
            public ScriptMemberInfo Target;
            public ScriptMemberInfo Source;
            public PropertiesListElement PropertiesList;
            public GroupElement Group;
            public bool Invert;
            public int LabelIndex;

            public abstract bool ShouldBeVisible(object instance);
        }

        private class VisibleIfCache : VisibleIfBaseCache {
            public override bool ShouldBeVisible(object instance)
            {
                var value = (bool)Source.GetValue(instance);
                if (Invert)
                    value = !value;
                return value;
            }
        }

        private class VisibleIfValueCache : VisibleIfBaseCache {
            public object Value;

            public override bool ShouldBeVisible(object instance)
            {
                var value = Source.GetValue(instance);
                var result = Equals(value, Value);

                return Invert ? !result : result;
            }
        }

        private static HashSet<PropertiesList> _visibleIfPropertiesListsCache;
        private static Stack<Dictionary<string, GroupElement>> _groups;
        private static List<Dictionary<string, GroupElement>> _groupsPool;
        private List<VisibleIfBaseCache> _visibleIfCaches;
        private bool _isNull;

        /// <summary>
        /// Gets the items for the type
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The items.</returns>
        protected virtual List<ItemInfo> GetItemsForType(ScriptType type)
        {
            return GetItemsForType(type, type.IsClass, true);
        }

        /// <summary>
        /// Gets the items for the type
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="useProperties">True if use type properties.</param>
        /// <param name="useFields">True if use type fields.</param>
        /// <returns>The items.</returns>
        protected List<ItemInfo> GetItemsForType(ScriptType type, bool useProperties, bool useFields)
        {
            var items = new List<ItemInfo>();

            if (useProperties)
            {
                // Process properties
                var properties = type.GetProperties(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance);
                items.Capacity = Math.Max(items.Capacity, items.Count + properties.Length);
                for (int i = 0; i < properties.Length; i++)
                {
                    var p = properties[i];
                    var attributes = p.GetAttributes(true);
                    var showInEditor = attributes.Any(x => x is ShowInEditorAttribute);

                    // Skip properties without getter or setter
                    if (!p.HasGet || (!p.HasSet && !showInEditor))
                        continue;

                    // Skip hidden fields, handle special attributes
                    if ((!p.IsPublic && !showInEditor) || attributes.Any(x => x is HideInEditorAttribute))
                        continue;

                    items.Add(new ItemInfo(p, attributes));
                }
            }

            if (useFields)
            {
                // Process fields
                var fields = type.GetFields(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance);
                items.Capacity = Math.Max(items.Capacity, items.Count + fields.Length);
                for (int i = 0; i < fields.Length; i++)
                {
                    var f = fields[i];

                    var attributes = f.GetAttributes(true);

                    // Skip hidden fields, handle special attributes
                    if ((!f.IsPublic && !attributes.Any(x => x is ShowInEditorAttribute)) || attributes.Any(x => x is HideInEditorAttribute))
                        continue;

                    items.Add(new ItemInfo(f, attributes));
                }
            }

            return items;
        }

        private static ScriptMemberInfo GetVisibleIfSource(ScriptType type, VisibleIfBaseAttribute visibleIf)
        {
            var property = type.GetProperty(visibleIf.MemberName, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            if (property != ScriptMemberInfo.Null)
            {
                if (!property.HasGet)
                {
                    Debug.LogError("Invalid VisibleIf rule. Property has missing getter " + visibleIf.MemberName);
                    return ScriptMemberInfo.Null;
                }

                if (property.ValueType.Type != visibleIf.MemberType)
                {
                    Debug.LogError("Invalid VisibleIf rule. Property has to return '" + nameof(visibleIf.MemberType) + "' type " + visibleIf.MemberName);
                    return ScriptMemberInfo.Null;
                }

                return property;
            }

            var field = type.GetField(visibleIf.MemberName, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            if (field != ScriptMemberInfo.Null)
            {
                if (field.ValueType.Type != visibleIf.MemberType)
                {
                    Debug.LogError("Invalid VisibleIf rule. Field '" + visibleIf.MemberName + "' has to be of '" + nameof(visibleIf.MemberType) + "' type");
                    return ScriptMemberInfo.Null;
                }

                return field;
            }

            Debug.LogError("Invalid VisibleIf rule. Cannot find member " + visibleIf.MemberName);
            return ScriptMemberInfo.Null;
        }

        private static void GroupPanelCheckIfCanRevert(LayoutElementsContainer layout, ref bool canRevertReference, ref bool canRevertDefault)
        {
            if (layout == null || canRevertReference && canRevertDefault)
                return;

            foreach (var editor in layout.Editors)
            {
                canRevertReference |= editor.CanRevertReferenceValue;
                canRevertDefault |= editor.CanRevertDefaultValue;
            }

            foreach (var child in layout.Children)
                GroupPanelCheckIfCanRevert(child as LayoutElementsContainer, ref canRevertReference, ref canRevertDefault);
        }

        private static void OnGroupPanelRevert(LayoutElementsContainer layout, bool toDefault)
        {
            if (layout == null)
                return;

            foreach (var editor in layout.Editors)
            {
                if (toDefault && editor.CanRevertDefaultValue)
                    editor.RevertToDefaultValue();
                else if (!toDefault && editor.CanRevertReferenceValue)
                    editor.RevertToReferenceValue();
            }

            foreach (var child in layout.Children)
                OnGroupPanelRevert(child as LayoutElementsContainer, toDefault);
        }

        private static void OnGroupPanelCopy(LayoutElementsContainer layout)
        {
            if (layout.Editors.Count == 1)
            {
                layout.Editors[0].Copy();
            }
            else if (layout.Editors.Count != 0)
            {
                var data = new string[layout.Editors.Count];
                var sb = new StringBuilder();
                sb.Append("[\n");
                for (var i = 0; i < layout.Editors.Count; i++)
                {
                    layout.Editors[i].Copy();
                    if (i != 0)
                        sb.Append(",\n");
                    sb.Append(Clipboard.Text);
                    data[i] = Clipboard.Text;
                }
                sb.Append("\n]");
                Clipboard.Text = sb.ToString();
                Clipboard.Text = JsonSerializer.Serialize(data);
            }
            else if (layout.Children.Any(x => x is LayoutElementsContainer))
            {
                foreach (var child in layout.Children)
                {
                    if (child is LayoutElementsContainer childContainer)
                    {
                        OnGroupPanelCopy(childContainer);
                        break;
                    }
                }
            }
        }

        private static bool OnGroupPanelCanCopy(LayoutElementsContainer layout)
        {
            return layout.Editors.Count != 0 || layout.Children.Any(x => x is LayoutElementsContainer);
        }

        private static void OnGroupPanelPaste(LayoutElementsContainer layout)
        {
            if (layout.Editors.Count == 1)
            {
                layout.Editors[0].Paste();
            }
            else if (layout.Editors.Count != 0)
            {
                var sb = Clipboard.Text;
                if (!string.IsNullOrEmpty(sb))
                {
                    try
                    {
                        var data = JsonSerializer.Deserialize<string[]>(sb);
                        if (data == null || data.Length != layout.Editors.Count)
                            return;
                        for (var i = 0; i < layout.Editors.Count; i++)
                        {
                            Clipboard.Text = data[i];
                            layout.Editors[i].Paste();
                        }
                    }
                    catch
                    {
                    }
                    finally
                    {
                        Clipboard.Text = sb;
                    }
                }
            }
            else if (layout.Children.Any(x => x is LayoutElementsContainer))
            {
                foreach (var child in layout.Children)
                {
                    if (child is LayoutElementsContainer childContainer)
                    {
                        OnGroupPanelPaste(childContainer);
                        break;
                    }
                }
            }
        }

        private static bool OnGroupPanelCanPaste(LayoutElementsContainer layout)
        {
            if (layout.Editors.Count == 1)
            {
                return layout.Editors[0].CanPaste;
            }
            if (layout.Editors.Count != 0)
            {
                var sb = Clipboard.Text;
                if (!string.IsNullOrEmpty(sb))
                {
                    try
                    {
                        var data = JsonSerializer.Deserialize<string[]>(sb);
                        if (data == null || data.Length != layout.Editors.Count)
                            return false;
                        for (var i = 0; i < layout.Editors.Count; i++)
                        {
                            Clipboard.Text = data[i];
                            if (!layout.Editors[i].CanPaste)
                                return false;
                        }
                        return true;
                    }
                    catch
                    {
                        return false;
                    }
                    finally
                    {
                        Clipboard.Text = sb;
                    }
                }
                return false;
            }
            if (layout.Children.Any(x => x is LayoutElementsContainer))
            {
                foreach (var child in layout.Children)
                {
                    if (child is LayoutElementsContainer childContainer)
                        return OnGroupPanelCanPaste(childContainer);
                }
            }
            return false;
        }

        private static void OnGroupPanelMouseButtonRightClicked(DropPanel groupPanel, Float2 location)
        {
            var group = (GroupElement)groupPanel.Tag;
            bool canRevertReference = false, canRevertDefault = false;
            GroupPanelCheckIfCanRevert(group, ref canRevertReference, ref canRevertDefault);

            var menu = new ContextMenu();
            var revertToPrefab = menu.AddButton("Revert to Prefab", () => OnGroupPanelRevert(group, false));
            revertToPrefab.Enabled = canRevertReference;
            var resetToDefault = menu.AddButton("Reset to default", () => OnGroupPanelRevert(group, true));
            resetToDefault.Enabled = canRevertDefault;
            menu.AddSeparator();
            var copy = menu.AddButton("Copy", () => OnGroupPanelCopy(group));
            copy.Enabled = OnGroupPanelCanCopy(group);
            var paste = menu.AddButton("Paste", () => OnGroupPanelPaste(group));
            paste.Enabled = OnGroupPanelCanPaste(group);

            menu.Show(groupPanel, location);
        }

        internal static void OnGroupsBegin()
        {
            if (_groups == null)
                _groups = new Stack<Dictionary<string, GroupElement>>();
            if (_groupsPool == null)
                _groupsPool = new List<Dictionary<string, GroupElement>>();
            Dictionary<string, GroupElement> group;
            if (_groupsPool.Count != 0)
            {
                group = _groupsPool[0];
                _groupsPool.RemoveAt(0);
            }
            else
            {
                group = new Dictionary<string, GroupElement>();
            }
            _groups.Push(group);
        }

        internal static void OnGroupsEnd()
        {
            var groups = _groups.Pop();
            groups.Clear();
            _groupsPool.Add(groups);
        }

        internal static LayoutElementsContainer OnGroup(LayoutElementsContainer layout, EditorDisplayAttribute display)
        {
            if (display?.Group != null)
            {
                var groups = _groups.Peek();
                if (groups.TryGetValue(display.Group, out var group))
                {
                    // Reuse group
                    layout = group;
                }
                else
                {
                    // Add new group
                    group = layout.Group(display.Group);
                    group.Panel.Tag = group;
                    group.Panel.MouseButtonRightClicked += OnGroupPanelMouseButtonRightClicked;
                    groups.Add(display.Group, group);
                    layout = group;
                }
            }
            return layout;
        }

        /// <summary>
        /// Spawns the property for the given item.
        /// </summary>
        /// <param name="itemLayout">The item layout.</param>
        /// <param name="itemValues">The item values.</param>
        /// <param name="item">The item.</param>
        protected virtual void SpawnProperty(LayoutElementsContainer itemLayout, ValueContainer itemValues, ItemInfo item)
        {
            int labelIndex = 0;
            if ((item.IsReadOnly || item.VisibleIf != null || item.VisibleIfValue != null) &&
                itemLayout.Children.Count > 0 &&
                itemLayout.Children[itemLayout.Children.Count - 1] is PropertiesListElement propertiesListElement)
            {
                labelIndex = propertiesListElement.Labels.Count;
            }

            itemLayout.Property(item.DisplayName, itemValues, item.OverrideEditor, item.TooltipText);

            if (item.IsReadOnly && itemLayout.Children.Count > 0)
            {
                PropertiesListElement list = null;
                int firstChildControlIndex = 0;
                bool disableSingle = true;
                var control = itemLayout.Children[itemLayout.Children.Count - 1];
                if (control is GroupElement group && group.Children.Count > 0)
                {
                    list = group.Children[0] as PropertiesListElement;
                    disableSingle = false; // Disable all nested editors
                }
                else if (control is PropertiesListElement list1)
                {
                    list = list1;
                    firstChildControlIndex = list.Labels[labelIndex].FirstChildControlIndex;
                }

                if (list != null)
                {
                    // Disable controls added to the editor
                    var count = list.Properties.Children.Count;
                    for (int j = firstChildControlIndex; j < count; j++)
                    {
                        var child = list.Properties.Children[j];
                        if (disableSingle && child is PropertyNameLabel)
                            break;

                        if (child != null)
                            child.Enabled = false;
                    }
                }
            }

            if (itemLayout.Children.Count > 0) {
                if (item.VisibleIf != null) {
                    AddVisibleIfCache<VisibleIfCache>(itemLayout, item, labelIndex, item.VisibleIf);
                }
                if (item.VisibleIfValue != null) {
                    var newCache = AddVisibleIfCache<VisibleIfValueCache>(itemLayout, item, labelIndex, item.VisibleIfValue);

                    if (newCache != null) {
                        newCache.Value = item.VisibleIfValue.Value;
                    }
                }
            }
        }

        private TVisibleIf AddVisibleIfCache<TVisibleIf>(LayoutElementsContainer itemLayout, ItemInfo item, int labelIndex, VisibleIfBaseAttribute visibleIfAttribute) where TVisibleIf : VisibleIfBaseCache, new() {
            PropertiesListElement list = null;
            GroupElement group = null;

            if (itemLayout.Children[itemLayout.Children.Count - 1] is PropertiesListElement list1)
                list = list1;
            else if (itemLayout.Children[itemLayout.Children.Count - 1] is GroupElement group1)
                group = group1;
            else
                return null;

            // Get source member used to check rule
            var sourceMember = GetVisibleIfSource(item.Info.DeclaringType, visibleIfAttribute);

            if (sourceMember == ScriptType.Null)
                return null;

            // Resize cache
            if (_visibleIfCaches == null)
                _visibleIfCaches = new List<VisibleIfBaseCache>();

            // Add item
            var result = new TVisibleIf {
                Target = item.Info,
                Source = sourceMember,
                PropertiesList = list,
                Group = group,
                LabelIndex = labelIndex,
                Invert = visibleIfAttribute.Invert
            };
            var emptyIndex = _visibleIfCaches.FindIndex(item => item?.Target == null || item.Target == ScriptType.Null);

            if (emptyIndex >= 0) {
                _visibleIfCaches[emptyIndex] = result;
            } else {
                _visibleIfCaches.Add(result);
            }

            return result;
        }

        /// <inheritdoc />
        internal override void Initialize(CustomEditorPresenter presenter, LayoutElementsContainer layout, ValueContainer values)
        {
            _isNull = values != null && values.IsNull;

            base.Initialize(presenter, layout, values);
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _visibleIfCaches = null;

            // Collect items to edit
            List<ItemInfo> items;
            if (!HasDifferentTypes)
            {
                var value = Values[0];
                if (value == null)
                {
                    // Check if it's an object type that can be created in editor
                    var type = Values.Type;
                    if (type != ScriptMemberInfo.Null && type.CanCreateInstance)
                    {
                        layout = layout.Space(20);

                        const float ButtonSize = 14.0f;
                        var button = new Button
                        {
                            Text = "+",
                            TooltipText = "Create a new instance of the object",
                            Size = new Float2(ButtonSize, ButtonSize),
                            AnchorPreset = AnchorPresets.MiddleRight,
                            Parent = layout.ContainerControl,
                            Location = new Float2(layout.ContainerControl.Width - ButtonSize - 4, (layout.ContainerControl.Height - ButtonSize) * 0.5f),
                        };
                        button.Clicked += () => SetValue(Values.Type.CreateInstance());
                    }

                    layout.Label("<null>");
                    return;
                }

                items = GetItemsForType(TypeUtils.GetObjectType(value));
            }
            else
            {
                var types = ValuesTypes;
                items = new List<ItemInfo>(GetItemsForType(types[0]));
                for (int i = 1; i < types.Length && items.Count > 0; i++)
                {
                    var otherItems = GetItemsForType(types[i]);

                    // Merge items
                    for (int j = 0; j < items.Count && items.Count > 0; j++)
                    {
                        bool isInvalid = true;
                        for (int k = 0; k < otherItems.Count; k++)
                        {
                            var a = items[j];
                            var b = otherItems[k];

                            if (ItemInfo.CanMerge(a, b))
                            {
                                isInvalid = false;
                                break;
                            }
                        }

                        if (isInvalid)
                        {
                            items.RemoveAt(j--);
                        }
                    }
                }
            }

            // Sort items
            items.Sort();

            // Add items
            OnGroupsBegin();
            for (int i = 0; i < items.Count; i++)
            {
                var item = items[i];

                // Group
                var itemLayout = OnGroup(layout, item.Display);

                // Space
                if (item.Space != null)
                    itemLayout.Space(item.Space.Height);

                // Header
                if (item.Header != null)
                    itemLayout.Header(item.Header);

                try
                {
                    // Peek values
                    ValueContainer itemValues = item.GetValues(Values);

                    // Spawn property editor
                    SpawnProperty(itemLayout, itemValues, item);
                }
                catch (Exception ex)
                {
                    Editor.LogWarning("Failed to setup values and UI for item " + item);
                    Editor.LogWarning(ex.Message);
                    Editor.LogWarning(ex.StackTrace);
                    return;
                }

                // Expand all parent groups if need to
                if (item.ExpandGroups)
                {
                    var c = itemLayout.ContainerControl;
                    do
                    {
                        if (c is DropPanel dropPanel)
                            dropPanel.Open(false);
                        else if (c is CustomEditorPresenter.PresenterPanel)
                            break;
                        c = c.Parent;
                    } while (c != null);
                }
            }
            OnGroupsEnd();
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            // Automatic refresh when value nullability changed
            if (_isNull != Values.IsNull)
            {
                RebuildLayout();
                return;
            }

            if (_visibleIfCaches != null)
            {
                if (_visibleIfPropertiesListsCache == null)
                    _visibleIfPropertiesListsCache = new HashSet<PropertiesList>();
                else
                    _visibleIfPropertiesListsCache.Clear();
                try
                {
                    // Update VisibleIf rules
                    foreach (var c in _visibleIfCaches) {
                        if (c.Target == ScriptMemberInfo.Null)
                            break;

                        // Check rule (all objects must allow to show this property)
                        bool visible = true;
                        for (int j = 0; j < Values.Count; j++)
                        {
                            if (Values[j] != null && !c.ShouldBeVisible(Values[j]))
                            {
                                visible = false;
                                break;
                            }
                        }

                        // Apply the visibility (note: there may be no label)
                        if (c.LabelIndex != -1 && c.PropertiesList != null && c.PropertiesList.Labels.Count > c.LabelIndex)
                        {
                            var label = c.PropertiesList.Labels[c.LabelIndex];
                            label.Visible = visible;
                            for (int j = label.FirstChildControlIndex; j < c.PropertiesList.Properties.Children.Count; j++)
                            {
                                var child = c.PropertiesList.Properties.Children[j];
                                if (child is PropertyNameLabel)
                                    break;

                                child.Visible = visible;
                            }
                        }
                        if (c.Group != null)
                        {
                            c.Group.Panel.Visible = visible;
                        }
                        if (c.PropertiesList != null)
                            _visibleIfPropertiesListsCache.Add(c.PropertiesList.Properties);
                    }

                    // Hide properties lists with all labels being hidden
                    foreach (var propertiesList in _visibleIfPropertiesListsCache)
                    {
                        propertiesList.Visible = propertiesList.Children.Any(c => c.Visible);
                    }

                    // Hide group panels with all properties lists hidden
                    foreach (var propertiesList in _visibleIfPropertiesListsCache)
                    {
                        if (propertiesList.Parent is DropPanel dropPanel)
                            dropPanel.Visible = propertiesList.Visible || !dropPanel.Children.All(c => c is PropertiesList && !c.Visible);
                    }
                }
                catch (Exception ex)
                {
                    Editor.LogWarning(ex);
                    Editor.LogError("Failed to update VisibleIf rules. " + ex.Message);

                    // Remove rules to prevent error in loop
                    _visibleIfCaches = null;
                }
            }

            base.Refresh();
        }
    }
}
