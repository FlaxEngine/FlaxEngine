using System;
using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEngine;

/// <summary>
/// The control reference interface.
/// </summary>
public interface IControlReference
{
    /// <summary>
    /// The UIControl.
    /// </summary>
    public UIControl UIControl { get; }
    
    /// <summary>
    /// The Control type
    /// </summary>
    /// <returns>The Control Type</returns>
    public Type GetControlType();
    
    /// <summary>
    /// A safe set of the UI Control. Will warn if Control is of the wrong type.
    /// </summary>
    /// <param name="uiControl">The UIControl to set.</param>
    public void Set(UIControl uiControl);
}

/// <summary>
/// ControlReference class.
/// </summary>
[Serializable]
public struct ControlReference<T> : IControlReference where T : Control
{
    [Serialize, ShowInEditor]
    private UIControl _uiControl;

    /// <summary>
    /// Default constructor for ControlReference;
    /// </summary>
    public ControlReference()
    {
        _uiControl = null;
    }

    /// <inheritdoc />
    public Type GetControlType()
    {
        return typeof(T);
    }

    /// <summary>
    /// The Control attached to the UI Control.
    /// </summary>
    [HideInEditor]
    public T Control
    {
        get
        {
            if (_uiControl != null && _uiControl.Control is T t)
                return t;
            else
            {
                Debug.LogWarning("Trying to get Control from ControlReference but UIControl is null, or UIControl.Control is null, or UIControl.Control is not the correct type.");
                return null;
            }
        }
    }

    /// <inheritdoc />
    public UIControl UIControl => _uiControl;

    /// <inheritdoc />
    public void Set(UIControl uiControl)
    {
        if (uiControl == null)
        {
            Clear();
            return;
        }
        
        if (uiControl.Control is T castedControl)
            _uiControl = uiControl;
        else
            Debug.LogWarning("Trying to set ControlReference but UIControl.Control is null or UIControl.Control is not the correct type.");
    }

    /// <summary>
    /// Clears the UIControl reference.
    /// </summary>
    public void Clear()
    {
        _uiControl = null;
    }

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
