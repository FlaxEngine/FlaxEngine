// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.SceneGraph;
using FlaxEditor.SceneGraph.GUI;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using Object = FlaxEngine.Object;

namespace FlaxEditor.CustomEditors.Editors;

/// <summary>
/// The reference picker control used for UIControls using ControlReference.
/// </summary>
public class UIControlRefPickerControl : Control
{
    private Type _controlType;
    private UIControl _value;
    private ActorTreeNode _linkedTreeNode;
    private string _valueName;
    private bool _supportsPickDropDown;

    private bool _isMouseDown;
    private Float2 _mouseDownPos;
    private Float2 _mousePos;

    private bool _hasValidDragOver;
    private DragActors _dragActors;
    private DragHandlers _dragHandlers;

    /// <summary>
    /// The presenter using this control.
    /// </summary>
    public IPresenterOwner PresenterContext;

    /// <summary>
    /// Occurs when value gets changed.
    /// </summary>
    public event Action ValueChanged;

    /// <summary>
    /// The type of the Control
    /// </summary>
    /// <exception cref="ArgumentException">Throws exception if value is not a subclass of control.</exception>
    public Type ControlType
    {
        get => _controlType;
        set
        {
            if (_controlType == value)
                return;
            if (!value.IsSubclassOf(typeof(Control)))
                throw new ArgumentException(string.Format("Invalid type for UIControlObjectRefPicker. Input type: {0}", value));
            _controlType = value;
            _supportsPickDropDown = typeof(Control).IsAssignableFrom(value);

            // Deselect value if it's not valid now
            if (!IsValid(_value))
                Value = null;
        }
    }

    /// <summary>
    /// The UIControl value.
    /// </summary>
    public UIControl Value
    {
        get => _value;
        set
        {
            if (_value == value)
                return;
            if (!IsValid(value))
                value = null;

            // Special case for missing objects (eg. referenced actor in script that is deleted in editor)
            if (value != null && (Object.GetUnmanagedPtr(value) == IntPtr.Zero || value.ID == Guid.Empty))
                value = null;

            _value = value;

            // Get name to display
            if (_value != null)
                _valueName = _value.Name;
            else
                _valueName = string.Empty;

            // Update tooltip
            if (_value is SceneObject sceneObject)
                TooltipText = Utilities.Utils.GetTooltip(sceneObject);
            else
                TooltipText = string.Empty;

            OnValueChanged();
        }
    }

    /// <summary>
    /// Gets or sets the selected object value by identifier.
    /// </summary>
    public Guid ValueID
    {
        get => _value ? _value.ID : Guid.Empty;
        set => Value = Object.Find<UIControl>(ref value);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="UIControlRefPickerControl"/> class.
    /// </summary>
    public UIControlRefPickerControl()
    : base(0, 0, 50, 16)
    {
    }

    private void OnValueChanged()
    {
        ValueChanged?.Invoke();
    }

    private void ShowDropDownMenu()
    {
        Focus();
        if (typeof(Control).IsAssignableFrom(_controlType))
        {
            ActorSearchPopup.Show(this, new Float2(0, Height), IsValid, actor =>
            {
                Value = actor as UIControl;
                RootWindow.Focus();
                Focus();
            }, PresenterContext);
        }
    }

    private bool IsValid(Actor actor)
    {
        return actor == null || actor is UIControl a && a.Control.GetType() == _controlType;
    }

    private bool ValidateDragActor(ActorNode a)
    {
        if (!IsValid(a.Actor))
            return false;

        if (PresenterContext is PrefabWindow prefabWindow)
        {
            if (prefabWindow.Tree == a.TreeNode.ParentTree)
                return true;
        }
        else if (PresenterContext is PropertiesWindow || PresenterContext == null)
        {
            if (a.ParentScene != null)
                return true;
        }
        return false;
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
            Render2D.DrawText(style.FontMedium, $"{_valueName} ({Utilities.Utils.GetPropertyNameUI(ControlType.GetTypeDisplayName())})", nameRect, isEnabled ? style.Foreground : style.ForegroundDisabled, TextAlignment.Near, TextAlignment.Center);
            Render2D.PopClip();

            // Draw deselect button
            Render2D.DrawSprite(style.Cross, button1Rect, isEnabled && button1Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);
        }
        else
        {
            // Draw info
            Render2D.PushClip(nameRect);
            Render2D.DrawText(style.FontMedium, ControlType != null ? $"None ({Utilities.Utils.GetPropertyNameUI(ControlType.GetTypeDisplayName())})" : "-", nameRect, isEnabled ? style.ForegroundGrey : style.ForegroundGrey.AlphaMultiplied(0.75f), TextAlignment.Near, TextAlignment.Center);
            Render2D.PopClip();
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
        {
            ShowDropDownMenu();
            return true;
        }

        if (button == MouseButton.Left)
        {
            _isMouseDown = false;

            // Highlight actor or script reference
            if (!_hasValidDragOver && !IsDragOver && nameRect.Contains(location))
            {
                Actor actor = _value;
                if (actor != null)
                {
                    if (_linkedTreeNode != null && _linkedTreeNode.Actor == actor)
                    {
                        _linkedTreeNode.ExpandAllParents();
                        _linkedTreeNode.StartHighlight();
                    }
                    else
                    {
                        _linkedTreeNode = Editor.Instance.Scene.GetActorNode(actor).TreeNode;
                        _linkedTreeNode.ExpandAllParents();
                        Editor.Instance.Windows.SceneWin.SceneTreePanel.ScrollViewTo(_linkedTreeNode, true);
                        _linkedTreeNode.StartHighlight();
                    }
                    return true;
                }
            }

            // Reset valid drag over if still true at this point
            if (_hasValidDragOver)
                _hasValidDragOver = false;
        }

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
            if (_linkedTreeNode != null)
            {
                _linkedTreeNode.StopHighlight();
                _linkedTreeNode = null;
            }

            // Select object
            if (_value is Actor actor)
                Editor.Instance.SceneEditing.Select(actor);
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
        }
    }

    private DragDropEffect DragEffect => _hasValidDragOver ? DragDropEffect.Move : DragDropEffect.None;

    /// <inheritdoc />
    public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
    {
        base.OnDragEnter(ref location, data);

        // Ensure to have valid drag helpers (uses lazy init)
        if (_dragActors == null)
            _dragActors = new DragActors(ValidateDragActor);
        if (_dragHandlers == null)
        {
            _dragHandlers = new DragHandlers
            {
                _dragActors,
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
            Value = _dragActors.Objects[0].Actor as UIControl;
        }

        return result;
    }

    /// <inheritdoc />
    public override void OnDestroy()
    {
        _value = null;
        _controlType = null;
        _valueName = null;
        _linkedTreeNode = null;

        base.OnDestroy();
    }
}

/// <summary>
/// ControlReferenceEditor class.
/// </summary>
[CustomEditor(typeof(ControlReference<>)), DefaultEditor]
public class ControlReferenceEditor : CustomEditor
{
    private CustomElement<UIControlRefPickerControl> _element;

    /// <inheritdoc />
    public override DisplayStyle Style => DisplayStyle.Inline;

    /// <inheritdoc />
    public override void Initialize(LayoutElementsContainer layout)
    {
        if (!HasDifferentTypes)
        {
            _element = layout.Custom<UIControlRefPickerControl>();
            if (ValuesTypes == null || ValuesTypes[0] == null)
            {
                Editor.LogWarning("ControlReference needs to be assigned in code.");
                return;
            }
            Type genType = ValuesTypes[0].GetGenericArguments()[0];
            if (typeof(Control).IsAssignableFrom(genType))
            {
                _element.CustomControl.PresenterContext = Presenter.Owner;
                _element.CustomControl.ControlType = genType;
            }
            _element.CustomControl.ValueChanged += () =>
            {
                Type genericType = ValuesTypes[0].GetGenericArguments()[0];
                if (typeof(Control).IsAssignableFrom(genericType))
                {
                    Type t = typeof(ControlReference<>);
                    Type tw = t.MakeGenericType(new Type[] { genericType });
                    var instance = Activator.CreateInstance(tw);
                    ((IControlReference)instance).UIControl = _element.CustomControl.Value;
                    SetValue(instance);
                }
            };
        }
    }

    /// <inheritdoc />
    public override void Refresh()
    {
        base.Refresh();

        if (!HasDifferentValues)
        {
            if (Values[0] is IControlReference cr)
                _element.CustomControl.Value = cr.UIControl;
        }
    }
}
