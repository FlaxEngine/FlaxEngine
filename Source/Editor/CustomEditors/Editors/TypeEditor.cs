// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using System.Reflection;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// A custom control type used to pick reference to <see cref="System.Type"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public class TypePickerControl : Control
    {
        private ScriptType _type;
        private ScriptType _value;
        private string _valueName;

        private Float2 _mousePos;

        private bool _hasValidDragOver;
        private DragActors _dragActors;
        private DragScripts _dragScripts;
        private DragHandlers _dragHandlers;

        /// <summary>
        /// Gets or sets the allowed type (given type and all sub classes). Must be <see cref="System.Type"/> type of any subclass.
        /// </summary>
        public ScriptType Type
        {
            get => _type;
            set
            {
                if (_type == value)
                    return;
                if (value == ScriptType.Null)
                    throw new ArgumentNullException(nameof(value));

                _type = value;

                // Deselect value if it's not valid now
                if (!IsValid(_value))
                    Value = ScriptType.Null;
            }
        }

        /// <summary>
        /// Gets or sets the selected types value.
        /// </summary>
        public ScriptType Value
        {
            get => _value;
            set
            {
                if (_value == value)
                    return;
                if (value != ScriptType.Null && !IsValid(value))
                    throw new ArgumentException(string.Format("Invalid type {0}. Checked base type {1}.", value.TypeName ?? "<null>", _type.TypeName ?? "<null>"));

                _value = value;

                // Get name to display
                if (_value)
                {
                    _valueName = _value.Name;
                    TooltipText = Surface.SurfaceUtils.GetVisualScriptTypeDescription(_value);
                }
                else
                {
                    _valueName = string.Empty;
                    TooltipText = string.Empty;
                }

                ValueChanged?.Invoke();
                TypePickerValueChanged?.Invoke(this);
            }
        }

        /// <summary>
        /// Gets or sets the selected type fullname (namespace and class).
        /// </summary>
        public string ValueTypeName
        {
            get => _value.TypeName ?? string.Empty;
            set => Value = TypeUtils.GetType(value);
        }

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action<TypePickerControl> TypePickerValueChanged;

        /// <summary>
        /// The custom callback for types validation. Cane be used to implement a rule for types to pick.
        /// </summary>
        public Func<ScriptType, bool> CheckValid;

        /// <summary>
        /// Initializes a new instance of the <see cref="TypePickerControl"/> class.
        /// </summary>
        public TypePickerControl()
        : base(0, 0, 50, 16)
        {
            _type = ScriptType.Object;
        }

        private bool IsValid(ScriptType obj)
        {
            return _type.IsAssignableFrom(obj) && (CheckValid == null || CheckValid(obj));
        }

        private void ShowDropDownMenu()
        {
            Focus();
            TypeSearchPopup.Show(this, new Float2(0, Height), IsValid, scriptType =>
            {
                Value = scriptType;
                RootWindow.Focus();
                Focus();
            });
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Cache data
            var style = Style.Current;
            bool isSelected = _value != ScriptType.Null;
            var frameRect = new Rectangle(0, 0, Width - 16, 16);
            if (isSelected && _type == ScriptType.Null)
                frameRect.Width -= 16;
            var nameRect = new Rectangle(2, 1, frameRect.Width - 4, 14);
            var button1Rect = new Rectangle(nameRect.Right + 2, 1, 14, 14);
            var button2Rect = new Rectangle(button1Rect.Right + 2, 1, 14, 14);

            // Draw frame
            Render2D.DrawRectangle(frameRect, IsMouseOver ? style.BorderHighlighted : style.BorderNormal);

            // Check if has item selected
            if (isSelected)
            {
                // Draw deselect button
                if (_type == ScriptType.Null)
                    Render2D.DrawSprite(style.Cross, button1Rect, button1Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);

                // Draw name
                Render2D.PushClip(nameRect);
                Render2D.DrawText(style.FontMedium, _valueName, nameRect, style.Foreground, TextAlignment.Near, TextAlignment.Center);
                Render2D.PopClip();
            }
            else
            {
                // Draw info
                Render2D.DrawText(style.FontMedium, "-", nameRect, Color.OrangeRed, TextAlignment.Near, TextAlignment.Center);
            }

            // Draw picker button
            var pickerRect = isSelected && _type == ScriptType.Null ? button2Rect : button1Rect;
            Render2D.DrawSprite(style.ArrowDown, pickerRect, pickerRect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);

            // Check if drag is over
            if (IsDragOver && _hasValidDragOver)
            {
                var bounds = new Rectangle(Float2.Zero, Size);
                Render2D.FillRectangle(bounds, style.Selection);
                Render2D.DrawRectangle(bounds, style.SelectionBorder);
            }
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            _mousePos = location;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            _mousePos = location;

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mousePos = Float2.Minimum;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            // Cache data
            bool isSelected = _value != ScriptType.Null;
            var frameRect = new Rectangle(0, 0, Width - 16, 16);
            if (isSelected && _type == ScriptType.Null)
                frameRect.Width -= 16;
            var nameRect = new Rectangle(2, 1, frameRect.Width - 4, 14);
            var button1Rect = new Rectangle(nameRect.Right + 2, 1, 14, 14);
            var button2Rect = new Rectangle(button1Rect.Right + 2, 1, 14, 14);

            // Deselect
            if (_value && button1Rect.Contains(ref location) && _type == ScriptType.Null)
                Value = ScriptType.Null;

            // Picker dropdown menu
            if ((isSelected && _type == ScriptType.Null ? button2Rect : button1Rect).Contains(ref location))
                ShowDropDownMenu();

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            // Navigate to types from game project
            if (button == MouseButton.Left && _value != ScriptType.Null)
            {
                var item = _value.ContentItem;
                if (item != null)
                    Editor.Instance.ContentEditing.Open(item);
            }

            return base.OnMouseDoubleClick(location, button);
        }

        private DragDropEffect DragEffect => _hasValidDragOver ? DragDropEffect.Move : DragDropEffect.None;

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            base.OnDragEnter(ref location, data);

            // Ensure to have valid drag helpers (uses lazy init)
            if (_dragActors == null)
                _dragActors = new DragActors(x => IsValid(TypeUtils.GetObjectType(x.Actor)));

            if (_dragScripts == null)
                _dragScripts = new DragScripts(x => IsValid(TypeUtils.GetObjectType(x)));

            if (_dragHandlers == null)
            {
                _dragHandlers = new DragHandlers
                {
                    _dragActors,
                    _dragScripts
                };
            }

            _hasValidDragOver = _dragHandlers.OnDragEnter(data) != DragDropEffect.None;

            return DragEffect;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            base.OnDragMove(ref location, data);

            return DragEffect;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            _hasValidDragOver = false;
            _dragHandlers.OnDragLeave();

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            var result = DragEffect;

            base.OnDragDrop(ref location, data);

            if (_dragActors.HasValidDrag)
            {
                Value = TypeUtils.GetObjectType(_dragActors.Objects[0].Actor);
            }
            else if (_dragScripts.HasValidDrag)
            {
                Value = TypeUtils.GetObjectType(_dragScripts.Objects[0]);
            }

            return result;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _value = ScriptType.Null;
            _type = ScriptType.Null;
            _valueName = null;

            base.OnDestroy();
        }
    }

    /// <summary>
    /// Base class for type reference editors.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.CustomEditor" />
    public class TypeEditorBase : CustomEditor
    {
        /// <summary>
        /// The element.
        /// </summary>
        protected CustomElement<TypePickerControl> _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            if (!HasDifferentTypes)
            {
                _element = layout.Custom<TypePickerControl>();

                var attributes = Values.GetAttributes();
                var typeReference = (TypeReferenceAttribute)attributes?.FirstOrDefault(x => x is TypeReferenceAttribute);
                if (typeReference != null)
                {
                    if (!string.IsNullOrEmpty(typeReference.TypeName))
                    {
                        var customType = TypeUtils.GetType(typeReference.TypeName);
                        if (customType)
                            _element.CustomControl.Type = customType;
                        else
                            Editor.LogError(string.Format("Unknown type '{0}' to use for type picker filter.", typeReference.TypeName));
                    }
                    if (!string.IsNullOrEmpty(typeReference.CheckMethod))
                    {
                        var parentType = ParentEditor.Values[0].GetType();
                        var method = parentType.GetMethod(typeReference.CheckMethod, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
                        if (method != null)
                        {
                            if (method.ReturnType == typeof(bool))
                            {
                                var methodParameters = method.GetParameters();
                                if (methodParameters.Length == 1 && methodParameters[0].ParameterType == typeof(ScriptType))
                                    _element.CustomControl.CheckValid += type => { return (bool)method.Invoke(ParentEditor.Values[0], new object[] { type }); };
                                else if (methodParameters.Length == 1 && methodParameters[0].ParameterType == typeof(Type))
                                    _element.CustomControl.CheckValid += type => { return type.Type != null && (bool)method.Invoke(ParentEditor.Values[0], new object[] { type.Type }); };
                                else
                                    Editor.LogError(string.Format("Unknown method '{0}' to use for type picker filter check function (object type: {1}). It must contain a single parameter of type System.Type.", typeReference.CheckMethod, parentType.FullName));
                            }
                            else
                                Editor.LogError(string.Format("Invalid method '{0}' to use for type picker filter check function (object type: {1}). It must return boolean value.", typeReference.CheckMethod, parentType.FullName));
                        }
                        else
                            Editor.LogError(string.Format("Unknown method '{0}' to use for type picker filter check function (object type: {1}).", typeReference.CheckMethod, parentType.FullName));
                    }
                }
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _element = null;
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit reference to the <see cref="System.Type"/>. Used to pick classes.
    /// </summary>
    [CustomEditor(typeof(Type)), DefaultEditor]
    public class TypeEditor : TypeEditorBase
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (_element != null)
            {
                _element.CustomControl.ValueChanged += () => SetValue(_element.CustomControl.Value.Type);
                if (_element.CustomControl.Type == ScriptType.Object)
                    _element.CustomControl.Type = Values.Type.Type != typeof(object) || Values[0] == null ? ScriptType.Object : TypeUtils.GetObjectType(Values[0]);
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues)
                _element.CustomControl.Value = new ScriptType(Values[0] as Type);
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit reference to the <see cref="FlaxEditor.Scripting.ScriptType"/>. Used to pick classes.
    /// </summary>
    [CustomEditor(typeof(ScriptType)), DefaultEditor]
    public class ScriptTypeEditor : TypeEditorBase
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (_element != null)
                _element.CustomControl.ValueChanged += () => SetValue(_element.CustomControl.Value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues)
                _element.CustomControl.Value = (ScriptType)Values[0];
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit reference to the <see cref="FlaxEngine.SoftTypeReference"/>. Used to pick classes.
    /// </summary>
    [CustomEditor(typeof(SoftTypeReference)), DefaultEditor]
    public class SoftTypeReferenceEditor : TypeEditorBase
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (_element != null)
                _element.CustomControl.ValueChanged += () => SetValue(new SoftTypeReference(_element.CustomControl.ValueTypeName));
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues)
                _element.CustomControl.ValueTypeName = ((SoftTypeReference)Values[0]).TypeName;
        }
    }

    /// <summary>
    /// Custom Editor implementation of the inspector used to edit reference but saved as string or text instead of hard reference to the type.
    /// </summary>
    public class TypeNameEditor : TypeEditorBase
    {
        /// <summary>
        /// Prevents spamming log if Value contains missing type to skip research in subsequential Refresh ticks.
        /// </summary>
        private string _lastTypeNameError;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (_element != null)
                _element.CustomControl.ValueChanged += OnValueChanged;
        }

        private void OnValueChanged()
        {
            var type = _element.CustomControl.Value;
            SetValue(type != ScriptType.Null ? type.TypeName : null);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues && Values[0] is string asTypename &&
                !string.Equals(asTypename, _lastTypeNameError, StringComparison.Ordinal))
            {
                try
                {
                    _element.CustomControl.Value = TypeUtils.GetType(asTypename);
                }
                finally
                {
                    if (_element.CustomControl.Value == null && asTypename.Length != 0)
                        _lastTypeNameError = asTypename;
                }
            }
        }
    }
}
