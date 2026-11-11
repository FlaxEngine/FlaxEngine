using System;
using FlaxEngine;

/// <summary>
/// Scene bootstrap fallback script. Attached to the scene root actor.
/// The heavy lifting is done by <see cref="Game"/>; this script only wires the UI canvas
/// once the scene starts (the canvas needs a parent actor at runtime).
/// </summary>
public class Main : Script
{
    private DemoPanel _panel;
    private UICanvas _canvas;

    /// <inheritdoc />
    public override void OnStart()
    {
        // Screen-space UI over the whole game viewport
        _canvas = Actor.GetOrAddChild<UICanvas>();
        if (_canvas != null)
        {
            _canvas.Name = "HUDCanvas";
            _panel = new DemoPanel();
            _canvas.GUI.AddChild(_panel);
        }
    }
}
