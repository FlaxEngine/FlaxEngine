// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// The custom combobox for enum editing. Supports some special cases for flag enums.
    /// </summary>
    /// <seealso cref="ComboBox" />
    public class EnumComboBox : ComboBox
    {
        /// <summary>
        /// The enum type.
        /// </summary>
        protected readonly Type _enumType;

        /// <summary>
        /// The enum entries. The same order as items in combo box.
        /// </summary>
        protected readonly List<Entry> _entries = new List<Entry>();

        /// <summary>
        /// The cached value from the UI.
        /// </summary>
        protected long _cachedValue;

        /// <summary>
        /// True if has value cached, otherwise false.
        /// </summary>
        protected bool _hasValueCached;

        /// <summary>
        /// The enum entry.
        /// </summary>
        public struct Entry
        {
            /// <summary>
            /// The name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The tooltip text.
            /// </summary>
            public string Tooltip;

            /// <summary>
            /// The value.
            /// </summary>
            public long Value;

            /// <summary>
            /// Initializes a new instance of the <see cref="Entry"/> struct.
            /// </summary>
            /// <param name="name">The name.</param>
            /// <param name="tooltip">The tooltip.</param>
            /// <param name="value">The value.</param>
            public Entry(string name, long value, string tooltip = null)
            {
                Name = name;
                Tooltip = tooltip;
                Value = value;
            }
        }

        /// <summary>
        /// Custom extension delegate used to build enum element entries layout.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="entries">The output entries collection.</param>
        public delegate void BuildEntriesDelegate(Type type, List<Entry> entries);

        /// <summary>
        /// Gets a value indicating whether this enum has flags.
        /// </summary>
        public bool IsFlags { get; }

        /// <summary>
        /// Gets or sets the value of the enum (may not be int).
        /// </summary>
        public object EnumTypeValue
        {
            get => Enum.ToObject(_enumType, Value);
            set => Value = Convert.ToInt64(value);
        }

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public long Value
        {
            get => _cachedValue;
            set
            {
                // Skip if won't change
                if (_cachedValue == value && _hasValueCached)
                    return;

                // Single value
                for (int i = 0; i < _entries.Count; i++)
                {
                    if (_entries[i].Value == value)
                    {
                        SelectedIndex = i;
                        return;
                    }
                }

                if (IsFlags)
                {
                    // Collection of flags
                    var selection = new List<int>();
                    for (int i = 0; i < _entries.Count; i++)
                    {
                        var e = _entries[i].Value;
                        if (e != 0 && (e & value) == e)
                        {
                            selection.Add(i);
                        }
                    }
                    Selection = selection;
                }
                else
                {
                    SelectedIndex = -1;
                }
            }
        }

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action<EnumComboBox> EnumValueChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="EnumElement"/> class.
        /// </summary>
        /// <param name="type">The enum type.</param>
        /// <param name="customBuildEntriesDelegate">The custom entries layout builder. Allows to hide existing or add different enum values to editor.</param>
        /// <param name="formatMode">The formatting mode.</param>
        public EnumComboBox(Type type, BuildEntriesDelegate customBuildEntriesDelegate = null, EnumDisplayAttribute.FormatMode formatMode = EnumDisplayAttribute.FormatMode.Default)
        {
            if (type == null)
                throw new ArgumentNullException(nameof(type));
            if (!type.IsEnum)
                throw new ArgumentException(string.Format("Invalid enum type {0}.", type.FullName));
            _enumType = type;
            IsFlags = _enumType.GetCustomAttribute<FlagsAttribute>() != null;

            SupportMultiSelect = IsFlags;
            MaximumItemsInViewCount = 15;
            SelectedIndexChanged += ComboBoxOnSelectedIndexChanged;

            if (customBuildEntriesDelegate != null)
                customBuildEntriesDelegate(type, _entries);
            else
                BuildEntriesDefault(type, _entries, formatMode);

            var hasTooltips = false;
            var entries = CollectionsMarshal.AsSpan(_entries);
            for (int i = 0; i < _entries.Count; i++)
            {
                ref var e = ref entries[i];
                _items.Add(e.Name);
                hasTooltips |= e.Tooltip != null;
            }

            if (hasTooltips)
            {
                var tooltips = new string[_entries.Count];
                for (int i = 0; i < _entries.Count; i++)
                {
                    ref var e = ref entries[i];
                    tooltips[i] = e.Tooltip;
                }
                _tooltips = tooltips;
            }
        }

        private void ComboBoxOnSelectedIndexChanged(ComboBox comboBox)
        {
            CacheValue();
            OnValueChanged();
        }

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            ValueChanged?.Invoke();
            EnumValueChanged?.Invoke(this);
        }

        /// <summary>
        /// Caches the selected UI enum value.
        /// </summary>
        protected void CacheValue()
        {
            long value = 0;
            if (IsFlags)
            {
                var selection = Selection;
                for (int i = 0; i < selection.Count; i++)
                {
                    var index = selection[i];
                    value |= _entries[index].Value;
                }
            }
            else
            {
                var selectedIndex = SelectedIndex;
                if (selectedIndex != -1)
                    value = _entries[selectedIndex].Value;
            }
            _cachedValue = value;
            _hasValueCached = true;
        }

        /// <summary>
        /// Builds the default entries for the given enum type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="entries">The output entries.</param>
        /// <param name="formatMode">The formatting mode.</param>
        public static void BuildEntriesDefault(Type type, List<Entry> entries, EnumDisplayAttribute.FormatMode formatMode = EnumDisplayAttribute.FormatMode.Default)
        {
            FieldInfo[] fields = type.GetFields();
            entries.Capacity = Mathf.Max(fields.Length - 1, entries.Capacity);
            for (int i = 0; i < fields.Length; i++)
            {
                var field = fields[i];
                if (field.Name.Equals("value__", StringComparison.Ordinal))
                    continue;

                var attributes = field.GetCustomAttributes(false);
                if (attributes.Any(x => x is HideInEditorAttribute))
                    continue;

                string name;
                var nameAttr = (EditorDisplayAttribute)attributes.FirstOrDefault(x => x is EditorDisplayAttribute);
                if (nameAttr != null)
                {
                    name = nameAttr.Name;
                }
                else
                {
                    switch (formatMode)
                    {
                    case EnumDisplayAttribute.FormatMode.Default:
                        name = Utilities.Utils.GetPropertyNameUI(field.Name);
                        break;
                    case EnumDisplayAttribute.FormatMode.None:
                        name = field.Name;
                        break;
                    default: throw new ArgumentOutOfRangeException(nameof(formatMode), formatMode, null);
                    }
                }

                string tooltip = Editor.Instance.CodeDocs.GetTooltip(new ScriptMemberInfo(field), attributes);

                entries.Add(new Entry(name, Convert.ToInt64(field.GetRawConstantValue()), tooltip));
            }
        }

        /// <inheritdoc />
        protected override void OnLayoutMenuButton(ContextMenuButton button, int index, bool construct = false)
        {
            base.OnLayoutMenuButton(button, index, construct);
            if (IsFlags)
                button.CloseMenuOnClick = false;
        }

        /// <inheritdoc />
        protected override void OnItemClicked(int index)
        {
            if (IsFlags)
            {
                var entries = _entries;

                // Special case if clicked enum with zero value
                if (entries[index].Value == 0)
                {
                    SelectedIndex = index;
                    return;
                }

                // Calculate value that will be set after change
                long valueAfter = 0;
                bool isSelected = _selectedIndices.Contains(index);
                long selectedValue = entries[index].Value;
                for (int i = 0; i < _selectedIndices.Count; i++)
                {
                    int selectedIndex = _selectedIndices[i];
                    if (selectedIndex != index && (isSelected || (entries[selectedIndex].Value & selectedValue) == 0))
                        valueAfter |= entries[selectedIndex].Value;
                }
                if (!isSelected)
                    valueAfter |= selectedValue;

                // Skip if value won't change
                if (Value == valueAfter)
                {
                    return;
                }

                // Build new selection
                for (int i = 0; i < entries.Count; i++)
                {
                    if (entries[i].Value == valueAfter)
                    {
                        SelectedIndex = i;
                        return;
                    }
                }
                _selectedIndices.Clear();
                for (int i = 0; i < entries.Count; i++)
                {
                    var e = entries[i].Value;
                    if (e != 0 && (e & valueAfter) == e)
                    {
                        _selectedIndices.Add(i);
                    }
                }
                OnSelectedIndexChanged();
                return;
            }

            base.OnItemClicked(index);
        }
    }
}
