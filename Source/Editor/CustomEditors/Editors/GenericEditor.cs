// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

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
            /// The tooltip attribute.
            /// </summary>
            public TooltipAttribute Tooltip;

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
            /// Gets a value indicating whether use dedicated group.
            /// </summary>
            public bool UseGroup => Display?.Group != null;

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
            public string TooltipText => Tooltip?.Text;

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
                Tooltip = (TooltipAttribute)attributes.FirstOrDefault(x => x is TooltipAttribute);
                CustomEditor = (CustomEditorAttribute)attributes.FirstOrDefault(x => x is CustomEditorAttribute);
                CustomEditorAlias = (CustomEditorAliasAttribute)attributes.FirstOrDefault(x => x is CustomEditorAliasAttribute);
                Space = (SpaceAttribute)attributes.FirstOrDefault(x => x is SpaceAttribute);
                Header = (HeaderAttribute)attributes.FirstOrDefault(x => x is HeaderAttribute);
                VisibleIf = (VisibleIfAttribute)attributes.FirstOrDefault(x => x is VisibleIfAttribute);
                IsReadOnly = attributes.FirstOrDefault(x => x is ReadOnlyAttribute) != null;
                ExpandGroups = attributes.FirstOrDefault(x => x is ExpandGroupsAttribute) != null;

                IsReadOnly |= !info.HasSet;
                DisplayName = Display?.Name ?? CustomEditorsUtil.GetPropertyNameUI(info.Name);
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

        private struct VisibleIfCache
        {
            public ScriptMemberInfo Target;
            public ScriptMemberInfo Source;
            public PropertiesListElement PropertiesList;
            public bool Invert;
            public int LabelIndex;

            public bool GetValue(object instance)
            {
                var value = (bool)Source.GetValue(instance);
                if (Invert)
                    value = !value;
                return value;
            }
        }

        private VisibleIfCache[] _visibleIfCaches;

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

                    // Skip properties without getter or setter
                    if (!p.HasGet || !p.HasSet)
                        continue;

                    var attributes = p.GetAttributes(true);

                    // Skip hidden fields, handle special attributes
                    if ((!p.IsPublic && !attributes.Any(x => x is ShowInEditorAttribute)) || attributes.Any(x => x is HideInEditorAttribute))
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

        private static ScriptMemberInfo GetVisibleIfSource(ScriptType type, VisibleIfAttribute visibleIf)
        {
            var property = type.GetProperty(visibleIf.MemberName, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            if (property != ScriptMemberInfo.Null)
            {
                if (!property.HasGet)
                {
                    Debug.LogError("Invalid VisibleIf rule. Property has missing getter " + visibleIf.MemberName);
                    return ScriptMemberInfo.Null;
                }

                if (property.ValueType.Type != typeof(bool))
                {
                    Debug.LogError("Invalid VisibleIf rule. Property has to return bool type " + visibleIf.MemberName);
                    return ScriptMemberInfo.Null;
                }

                return property;
            }

            var field = type.GetField(visibleIf.MemberName, BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
            if (field != ScriptMemberInfo.Null)
            {
                if (field.ValueType.Type != typeof(bool))
                {
                    Debug.LogError("Invalid VisibleIf rule. Field has to be bool type " + visibleIf.MemberName);
                    return ScriptMemberInfo.Null;
                }

                return field;
            }

            Debug.LogError("Invalid VisibleIf rule. Cannot find member " + visibleIf.MemberName);
            return ScriptMemberInfo.Null;
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
            if ((item.IsReadOnly || item.VisibleIf != null) &&
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

                        child.Enabled = false;
                    }
                }
            }
            if (item.VisibleIf != null)
            {
                PropertiesListElement list;
                if (itemLayout.Children.Count > 0 && itemLayout.Children[itemLayout.Children.Count - 1] is PropertiesListElement list1)
                {
                    list = list1;
                }
                else
                {
                    // TODO: support inlined objects hiding?
                    return;
                }

                // Get source member used to check rule
                var sourceMember = GetVisibleIfSource(item.Info.DeclaringType, item.VisibleIf);
                if (sourceMember == ScriptType.Null)
                    return;

                // Find the target control to show/hide

                // Resize cache
                if (_visibleIfCaches == null)
                    _visibleIfCaches = new VisibleIfCache[8];
                int count = 0;
                while (count < _visibleIfCaches.Length && _visibleIfCaches[count].Target != ScriptType.Null)
                    count++;
                if (count >= _visibleIfCaches.Length)
                    Array.Resize(ref _visibleIfCaches, count * 2);

                // Add item
                _visibleIfCaches[count] = new VisibleIfCache
                {
                    Target = item.Info,
                    Source = sourceMember,
                    PropertiesList = list,
                    LabelIndex = labelIndex,
                    Invert = item.VisibleIf.Invert,
                };
            }
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
                            Size = new Vector2(ButtonSize, ButtonSize),
                            AnchorPreset = AnchorPresets.MiddleRight,
                            Parent = layout.ContainerControl,
                            Location = new Vector2(layout.ContainerControl.Width - ButtonSize - 4, (layout.ContainerControl.Height - ButtonSize) * 0.5f),
                        };
                        button.Clicked += () =>
                        {
                            var newType = Values.Type;
                            SetValue(newType.CreateInstance());
                            if (ParentEditor != null)
                                ParentEditor.RebuildLayoutOnRefresh();
                            else
                                RebuildLayoutOnRefresh();
                        };
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
            GroupElement lastGroup = null;
            for (int i = 0; i < items.Count; i++)
            {
                var item = items[i];

                // Check if use group
                LayoutElementsContainer itemLayout;
                if (item.UseGroup)
                {
                    if (lastGroup == null || lastGroup.Panel.HeaderText != item.Display.Group)
                        lastGroup = layout.Group(item.Display.Group);
                    itemLayout = lastGroup;
                }
                else
                {
                    lastGroup = null;
                    itemLayout = layout;
                }

                // Space
                if (item.Space != null)
                    itemLayout.Space(item.Space.Height);

                // Header
                if (item.Header != null)
                    itemLayout.Header(item.Header.Text);

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
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            if (_visibleIfCaches != null)
            {
                try
                {
                    for (int i = 0; i < _visibleIfCaches.Length; i++)
                    {
                        var c = _visibleIfCaches[i];

                        if (c.Target == ScriptMemberInfo.Null)
                            break;

                        // Check rule (all objects must allow to show this property)
                        bool visible = true;
                        for (int j = 0; j < Values.Count; j++)
                        {
                            if (Values[j] != null && !c.GetValue(Values[j]))
                            {
                                visible = false;
                                break;
                            }
                        }

                        // Apply the visibility (note: there may be no label)
                        if (c.LabelIndex != -1 && c.PropertiesList.Labels.Count > c.LabelIndex)
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
