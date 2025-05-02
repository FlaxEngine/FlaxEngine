// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEngine;
using FlaxEditor.CustomEditors.Dedicated;
using FlaxEditor.CustomEditors;
using FlaxEditor.Scripting;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Dedicated custom editor for BoxCollider objects.
    /// </summary>
    [CustomEditor(typeof(BoxCollider)), DefaultEditor]
    public class BoxColliderEditor : ActorEditor
    {
        private bool _keepLocalOrientation = true;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            var group = layout.Group("Utility");
            var checkbox = group.Checkbox("Keep Local Orientation", "Keeps the local orientation when resizing.").CheckBox;
            checkbox.Checked = _keepLocalOrientation;
            checkbox.StateChanged += box => _keepLocalOrientation = box.Checked;
            group.Button("Resize to Fit", Editor.Instance.CodeDocs.GetTooltip(new ScriptMemberInfo(typeof(BoxCollider).GetMethod("AutoResize")))).Button.Clicked += OnResizeClicked;
            // This adds a bit of space between the button and the bottom of the group - even with a height of 0
            group.Space(0);
        }

        private void OnResizeClicked()
        {
            foreach (var value in Values)
            {
                if (value is BoxCollider collider)
                    collider.AutoResize(!_keepLocalOrientation);
            }
        }
    }

    /// <summary>
    /// Scene tree node for <see cref="BoxCollider"/> actor type.
    /// </summary>
    /// <seealso cref="ColliderNode" />
    [HideInEditor]
    public sealed class BoxColliderNode : ColliderNode
    {
        /// <inheritdoc />
        public BoxColliderNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            // Pick wires
            var actor = (BoxCollider)_actor;
            var box = actor.OrientedBox;
            if (Utilities.Utils.RayCastWire(ref box, ref ray.Ray, out distance, ref ray.View.Position))
            {
                normal = Vector3.Up;
                return true;
            }

            return base.RayCastSelf(ref ray, out distance, out normal);
        }

        /// <inheritdoc />
        public override void PostSpawn()
        {
            base.PostSpawn();

            if (Actor.HasPrefabLink)
            {
                return;
            }

            ((BoxCollider)Actor).AutoResize(false);
        }
    }
}
