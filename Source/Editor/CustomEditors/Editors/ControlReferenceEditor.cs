// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.CustomEditors.Editors;

/// <summary>
/// The reference picker control used for UIControls using ControlReference.
/// </summary>
internal class UIControlRefPickerControl : FlaxObjectRefPickerControl
{
    /// <summary>
    /// Type of the control to pick.
    /// </summary>
    public Type ControlType;

    /// <inheritdoc />
    protected override bool IsValid(Object obj)
    {
        return obj == null || (obj is UIControl control && control.Control.GetType() == ControlType);
    }
}

/// <summary>
/// ControlReferenceEditor class.
/// </summary>
[CustomEditor(typeof(ControlReference<>)), DefaultEditor]
internal class ControlReferenceEditor : CustomEditor
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
                _element.CustomControl.Type = new ScriptType(typeof(UIControl));
            }
            _element.CustomControl.ValueChanged += () =>
            {
                Type genericType = ValuesTypes[0].GetGenericArguments()[0];
                if (typeof(Control).IsAssignableFrom(genericType))
                {
                    Type t = typeof(ControlReference<>);
                    Type tw = t.MakeGenericType(new Type[] { genericType });
                    var instance = Activator.CreateInstance(tw);
                    ((IControlReference)instance).UIControl = (UIControl)_element.CustomControl.Value;
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
