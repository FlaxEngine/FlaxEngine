using System;
using System.Linq;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Options;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport
{
    /// <summary>
    /// Editor viewports base class.
    /// </summary>
    /// <seealso cref="T:FlaxEngine.GUI.RenderOutputControl" />
    public partial class EditorViewport
    {

        private readonly CameraViewpoint[] EditorViewportCameraViewpointValues = new CameraViewpoint[]
        {
            new CameraViewpoint("Front", new Float3(0f, 180f, 0f)),
            new CameraViewpoint("Back", new Float3(0f, 0f, 0f)),
            new CameraViewpoint("Left", new Float3(0f, 90f, 0f)),
            new CameraViewpoint("Right", new Float3(0f, -90f, 0f)),
            new CameraViewpoint("Top", new Float3(90f, 0f, 0f)),
            new CameraViewpoint("Bottom", new Float3(-90f, 0f, 0f))
        };

        private struct CameraViewpoint
        {
            public CameraViewpoint(string name, Vector3 orientation)
            {
                Name = name;
                Orientation = orientation;
            }

            public readonly string Name;

            public readonly Float3 Orientation;
        }
    }
}
