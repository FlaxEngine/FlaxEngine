// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Camera"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class CameraNode : ActorNode
    {
        /// <inheritdoc />
        public CameraNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void OnContextMenu(ContextMenu contextMenu, EditorWindow window)
        {
            base.OnContextMenu(contextMenu, window);
            if (window is not SceneTreeWindow win)
                return;
            var button = new ContextMenuButton(contextMenu, "Move Camera to View");
            button.Parent = contextMenu.ItemsContainer;
            contextMenu.ItemsContainer.Children.Remove(button);
            contextMenu.ItemsContainer.Children.Insert(4, button);
            button.Clicked += () =>
            {
                var c = Actor as Camera;
                var viewport = Editor.Instance.Windows.EditWin.Viewport;
                c.Position = viewport.ViewPosition;
                c.Orientation = viewport.ViewOrientation;
            };
        }

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            normal = Vector3.Up;

            // Check if skip raycasts
            if ((ray.Flags & RayCastData.FlagTypes.SkipEditorPrimitives) == RayCastData.FlagTypes.SkipEditorPrimitives)
            {
                distance = 0;
                return false;
            }

            return Camera.Internal_IntersectsItselfEditor(FlaxEngine.Object.GetUnmanagedPtr(_actor), ref ray.Ray, out distance);
        }

        /// <inheritdoc />
        public override Vector3[] GetActorSelectionPoints()
        {
            return [Actor.Position];
        }
    }
}
