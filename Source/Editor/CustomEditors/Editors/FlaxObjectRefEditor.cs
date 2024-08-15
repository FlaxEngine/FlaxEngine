// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using Object = FlaxEngine.Object;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// A custom control type used to pick reference to <see cref="FlaxEngine.Object"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public class FlaxObjectRefPickerControl : Control
    {
        private ScriptType _type;
        private Object _value;
        private string _valueName;
        private bool _supportsPickDropDown;

        private bool _isMouseDown;
        private Float2 _mouseDownPos;
        private Float2 _mousePos;

        private bool _hasValidDragOver;
        private DragActors _dragActors;
        private DragActors _dragActorsWithScript;
        private DragAssets _dragAssets;
        private DragScripts _dragScripts;
        private DragHandlers _dragHandlers;

        /// <summary>
        /// Gets or sets the allowed objects type (given type and all sub classes). Must be <see cref="Object"/> type of any subclass.
        /// </summary>
        public ScriptType Type
        {
            get => _type;
            set
            {
                if (_type == value)
                    return;
                if (value == ScriptType.Null || (value.Type != typeof(Object) && !value.IsSubclassOf(ScriptType.Object)))
                    throw new ArgumentException(string.Format("Invalid type for FlaxObjectRefEditor. Input type: {0}", value != ScriptType.Null ? value.TypeName : "null"));

                _type = value;
                _supportsPickDropDown = new ScriptType(typeof(Actor)).IsAssignableFrom(value) || new ScriptType(typeof(Script)).IsAssignableFrom(value);

                // Deselect value if it's not valid now
                if (!IsValid(_value))
                    Value = null;
            }
        }

        /// <summary>
        /// Gets or sets the selected object value.
        /// </summary>
        public Object Value
        {
            get => _value;
            set
            {
                if (_value == value)
                    return;
                if (!IsValid(value))
                    throw new ArgumentException("Invalid object type.");

                // Special case for missing objects (eg. referenced actor in script that is deleted in editor)
                if (value != null && (Object.GetUnmanagedPtr(value) == IntPtr.Zero || value.ID == Guid.Empty))
                    value = null;

                _value = value;
                var type = TypeUtils.GetObjectType(_value);

                // Get name to display
                if (_value is Script script)
                {
                    _valueName = script.Actor ? $"{type.Name} ({script.Actor.Name})" : type.Name;
                }
                else if (_value != null)
                {
                    _valueName = _value.ToString();
                }
                else
                {
                    _valueName = string.Empty;
                }

                // Update tooltip
                if (_value is SceneObject sceneObject)
                {
                    TooltipText = Utilities.Utils.GetTooltip(sceneObject);
                }
                else
                {
                    TooltipText = string.Empty;
                }

                OnValueChanged();
            }
        }

        /// <summary>
        /// Gets or sets the selected object value by identifier.
        /// </summary>
        public Guid ValueID
        {
            get => _value ? _value.ID : Guid.Empty;
            set => Value = Object.Find<Object>(ref value);
        }

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// The custom callback for objects validation. Cane be used to implement a rule for objects to pick.
        /// </summary>
        public Func<Object, ScriptType, bool> CheckValid;

        /// <summary>
        /// Initializes a new instance of the <see cref="FlaxObjectRefPickerControl"/> class.
        /// </summary>
        public FlaxObjectRefPickerControl()
        : base(0, 0, 50, 16)
        {
            _type = ScriptType.Object;
        }

        private bool IsValid(Object obj)
        {
            var type = TypeUtils.GetObjectType(obj);
            return obj == null || _type.IsAssignableFrom(type) && (CheckValid == null || CheckValid(obj, type));
        }

        private void ShowDropDownMenu()
        {
            Focus();
            if (new ScriptType(typeof(Actor)).IsAssignableFrom(_type))
            {
                ActorSearchPopup.Show(this, new Float2(0, Height), IsValid, actor =>
                {
                    Value = actor;
                    RootWindow.Focus();
                    Focus();
                });
            }
            else
            {
                ScriptSearchPopup.Show(this, new Float2(0, Height), IsValid, script =>
                {
                    Value = script;
                    RootWindow.Focus();
                    Focus();
                });
            }
        }

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            ValueChanged?.Invoke();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Cache data
            var style = Style.Current;
            bool isSelected = _value != null;
            bool isEnabled = EnabledInHierarchy;
            var frameRect = new Rectangle(0, 0, Width, 16);
            if (isSelected)
                frameRect.Width -= 16;
            if (_supportsPickDropDown)
                frameRect.Width -= 16;
            var nameRect = new Rectangle(2, 1, frameRect.Width - 4, 14);
            var button1Rect = new Rectangle(nameRect.Right + 2, 1, 14, 14);
            var button2Rect = new Rectangle(button1Rect.Right + 2, 1, 14, 14);

            // Draw frame
            Render2D.DrawRectangle(frameRect, isEnabled && (IsMouseOver || IsNavFocused) ? style.BorderHighlighted : style.BorderNormal);

            // Check if has item selected
            if (isSelected)
            {
                // Draw name
                Render2D.PushClip(nameRect);
                Render2D.DrawText(style.FontMedium, _valueName, nameRect, isEnabled ? style.Foreground : style.ForegroundDisabled, TextAlignment.Near, TextAlignment.Center);
                Render2D.PopClip();

                // Draw deselect button
                Render2D.DrawSprite(style.Cross, button1Rect, isEnabled && button1Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);
            }
            else
            {
                // Draw info
                Render2D.DrawText(style.FontMedium, Type != null ? $"None ({Utilities.Utils.GetPropertyNameUI(Type.ToString())})" : "-", nameRect, isEnabled ? style.ForegroundGrey : style.ForegroundGrey.AlphaMultiplied(0.75f), TextAlignment.Near, TextAlignment.Center);
            }

            // Draw picker button
            if (_supportsPickDropDown)
            {
                var pickerRect = isSelected ? button2Rect : button1Rect;
                Render2D.DrawSprite(style.ArrowDown, pickerRect, isEnabled && pickerRect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);
            }

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
            _mouseDownPos = Float2.Minimum;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mousePos = Float2.Minimum;

            // Check if start drag drop
            if (_isMouseDown)
            {
                // Do the drag
                DoDrag();

                // Clear flag
                _isMouseDown = false;
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            _mousePos = location;

            // Check if start drag drop
            if (_isMouseDown && Float2.Distance(location, _mouseDownPos) > 10.0f)
            {
                // Do the drag
                DoDrag();

                // Clear flag
                _isMouseDown = false;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                // Clear flag
                _isMouseDown = false;
            }

            // Cache data
            bool isSelected = _value != null;
            var frameRect = new Rectangle(0, 0, Width, 16);
            if (isSelected)
                frameRect.Width -= 16;
            if (_supportsPickDropDown)
                frameRect.Width -= 16;
            var nameRect = new Rectangle(2, 1, frameRect.Width - 4, 14);
            var button1Rect = new Rectangle(nameRect.Right + 2, 1, 14, 14);
            var button2Rect = new Rectangle(button1Rect.Right + 2, 1, 14, 14);

            // Deselect
            if (_value != null && button1Rect.Contains(ref location))
                Value = null;

            // Picker dropdown menu
            if (_supportsPickDropDown && (isSelected ? button2Rect : button1Rect).Contains(ref location))
                ShowDropDownMenu();

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                // Set flag
                _isMouseDown = true;
                _mouseDownPos = location;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            Focus();

            // Check if has object selected
            if (_value != null)
            {
                // Select object
                if (_value is Actor actor)
                    Editor.Instance.SceneEditing.Select(actor);
                else if (_value is Script script && script.Actor)
                    Editor.Instance.SceneEditing.Select(script.Actor);
                else if (_value is Asset asset)
                    Editor.Instance.Windows.ContentWin.Select(asset);
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            base.OnSubmit();

            // Picker dropdown menu
            if (_supportsPickDropDown)
                ShowDropDownMenu();
        }

        private void DoDrag()
        {
            // Do the drag drop operation if has selected element
            if (_value != null)
            {
                if (_value is Actor actor)
                    DoDragDrop(DragActors.GetDragData(actor));
                else if (_value is Asset asset)
                    DoDragDrop(DragAssets.GetDragData(asset));
                else if (_value is Script script)
                    DoDragDrop(DragScripts.GetDragData(script));
            }
        }

        private DragDropEffect DragEffect => _hasValidDragOver ? DragDropEffect.Move : DragDropEffect.None;

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            base.OnDragEnter(ref location, data);

            // Ensure to have valid drag helpers (uses lazy init)
            if (_dragActors == null)
                _dragActors = new DragActors(x => IsValid(x.Actor));
            if (_dragActorsWithScript == null)
                _dragActorsWithScript = new DragActors(ValidateDragActorWithScript);
            if (_dragAssets == null)
                _dragAssets = new DragAssets(ValidateDragAsset);
            if (_dragScripts == null)
                _dragScripts = new DragScripts(IsValid);
            if (_dragHandlers == null)
            {
                _dragHandlers = new DragHandlers
                {
                    _dragActors,
                    _dragActorsWithScript,
                    _dragAssets,
                    _dragScripts,
                };
            }

            _hasValidDragOver = _dragHandlers.OnDragEnter(data) != DragDropEffect.None;

            // Special case when dragging the actor with script to link script reference
            if (_dragActorsWithScript.HasValidDrag)
            {
                var script = _dragActorsWithScript.Objects[0].Actor.Scripts.First(IsValid);
                _dragActorsWithScript.Objects.Clear();
                _dragScripts.Objects.Add(script);
            }

            return DragEffect;
        }

        private bool ValidateDragAsset(AssetItem assetItem)
        {
            // Check if can accept assets
            if (!new ScriptType(typeof(Asset)).IsAssignableFrom(_type))
                return false;

            // Load or get asset
            var id = assetItem.ID;
            var obj = Object.Find<Asset>(ref id);
            if (obj == null)
                return false;

            // Check it
            return IsValid(obj);
        }

        private bool ValidateDragActorWithScript(ActorNode node)
        {
            return node.Actor.Scripts.Any(IsValid);
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
                Value = _dragActors.Objects[0].Actor;
            }
            else if (_dragAssets.HasValidDrag)
            {
                ValueID = _dragAssets.Objects[0].ID;
            }
            else if (_dragScripts.HasValidDrag)
            {
                ValueID = _dragScripts.Objects[0].ID;
            }

            return result;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _value = null;
            _type = ScriptType.Null;
            _valueName = null;

            base.OnDestroy();
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit reference to the <see cref="FlaxEngine.Object"/>.
    /// </summary>
    [CustomEditor(typeof(Object)), DefaultEditor]
    public sealed class FlaxObjectRefEditor : CustomEditor
    {
        private CustomElement<FlaxObjectRefPickerControl> _element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            if (!HasDifferentTypes)
            {
                _element = layout.Custom<FlaxObjectRefPickerControl>();
                _element.CustomControl.Type = Values.Type.Type != typeof(object) || Values[0] == null ? Values.Type : TypeUtils.GetObjectType(Values[0]);
                _element.CustomControl.ValueChanged += () => SetValue(_element.CustomControl.Value);
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues)
            {
                _element.CustomControl.Value = Values[0] as Object;
            }
        }
    }
}
