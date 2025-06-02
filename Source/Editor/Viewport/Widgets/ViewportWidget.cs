using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Viewport;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Linq;
using System;
using System.Xml;
using System.Reflection;

public abstract class ViewportWidget : Control
{

    protected ViewportWidget(float x, float y, float width, float height, EditorViewport viewport, ContextMenu contextMenu, bool attachTo = true, bool onAttach = true)
        : base(x, y, width, height)
    {
        if (attachTo)
            AttachTo(viewport, contextMenu, onAttach);
    }

    protected ViewportWidget(float x, float y, float width, float height, EditorViewport viewport, bool attachTo = true, bool onAttach = true)
        : base(x, y, width, height)
    {
        if (attachTo)
            AttachTo(viewport, onAttach);
    }

    public EditorViewport Viewport { get; private set; }
    public ContextMenu ContextMenu { get; private set; }

    public virtual void AttachTo(EditorViewport viewport, ContextMenu contextMenu, bool onAttach = true)
    {
        Viewport = viewport;
        Parent = viewport.Parent;
        ContextMenu = contextMenu;
        if (onAttach)
            OnAttach();
    }

    public virtual void AttachTo(EditorViewport viewport, bool onAttach = true)
    {
        Viewport = viewport;
        Parent = viewport.Parent;
        if (onAttach)
            OnAttach();
    }

    protected virtual void OnAttach()
    {
    }
}