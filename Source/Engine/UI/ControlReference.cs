// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.GUI;

namespace FlaxEngine;

/// <summary>
/// Interface for control references access.
/// </summary>
public interface IControlReference
{
    /// <summary>
    /// Gets or sets the reference to <see cref="FlaxEngine.UIControl"/> actor.
    /// </summary>
    public UIControl UIControl { get; set; }

    /// <summary>
    /// Gets the type of the control the interface uses.
    /// </summary>
    public Type ControlType { get; }
}

/// <summary>
/// UI Control reference utility. References UI Control actor with a typed control type.
/// </summary>
[Serializable]
public struct ControlReference<T> : IControlReference where T : Control
{
    private UIControl _uiControl;

    /// <summary>
    /// Gets the typed UI control object owned by the referenced <see cref="FlaxEngine.UIControl"/> actor.
    /// </summary>
    [HideInEditor]
    public T Control
    {
        get
        {
            if (_uiControl != null && _uiControl.Control is T t)
                return t;
            Debug.Write(LogType.Warning, "Trying to get Control from ControlReference but UIControl is null, or UIControl.Control is null, or UIControl.Control is not the correct type.");
            return null;
        }
    }

    /// <inheritdoc />
    public UIControl UIControl
    {
        get => _uiControl;
        set
        {
            if (value == null)
            {
                _uiControl = null;
            }
            else if (value.Control is T t)
            {
                _uiControl = value;
            }
            else
            {
                Debug.Write(LogType.Warning, "Trying to set ControlReference but UIControl.Control is null or UIControl.Control is not the correct type.");
            }
        }
    }

    /// <inheritdoc />
    public Type ControlType => typeof(T);

    /// <summary>
    /// The implicit operator for the Control.
    /// </summary>
    /// <param name="reference">The ControlReference</param>
    /// <returns>The Control.</returns>
    public static implicit operator T(ControlReference<T> reference) => reference.Control;

    /// <summary>
    /// The implicit operator for the UIControl
    /// </summary>
    /// <param name="reference">The ControlReference</param>
    /// <returns>The UIControl.</returns>
    public static implicit operator UIControl(ControlReference<T> reference) => reference.UIControl;
}
