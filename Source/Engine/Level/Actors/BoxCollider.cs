#if FLAX_EDITOR
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Dedicated;
#endif

namespace FlaxEngine
{
#if FLAX_EDITOR
    /// <summary>
    /// Dedicated custom editor for BoxCollider objects.
    /// </summary>
    [CustomEditor(typeof(BoxCollider)), DefaultEditor]
    public class BoxColliderEditor : ActorEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);
            layout.Space(20f);
            var autoResizeButton = layout.Button("Resize to Fit", "Resize the box collider to fit it's parent's bounds.");

            BoxCollider collider = Values[0] as BoxCollider;
            autoResizeButton.Button.Clicked += collider.AutoResize;
        }
    }
#endif

    partial class BoxCollider
    {
        private void BoxExcluding(Actor target, ref BoundingBox output, Actor excluded)
        {
            foreach (Actor child in target.Children)
            {
                if (child == excluded)
                {
                    continue;
                }

                output = BoundingBox.Merge(output, child.Box);
                BoxExcluding(child, ref output, excluded);
            }
        }

        /// <summary>
        /// Resizes the box collider based on the bounds of it's parent.
        /// </summary>
        public void AutoResize()
        {
            if (Parent is Scene)
            {
                return;
            }

            LocalPosition = Vector3.Zero;

            Vector3 parentScale = Parent.Scale;
            BoundingBox parentBox = Parent.Box;
            BoxExcluding(Parent, ref parentBox, this);

            Vector3 parentSize = parentBox.Size;
            Vector3 parentCenter = parentBox.Center - Parent.Position;

            // Avoid division by zero
            if (parentScale.X == 0 || parentScale.Y == 0 || parentScale.Z == 0)
            {
                return;
            }

            Size = parentSize / parentScale;
            Center = parentCenter / parentScale;

            // Undo Rotation
            Orientation *= Quaternion.Invert(Orientation);
        }

        /// <inheritdoc />
        public override void OnActorSpawned()
        {
            base.OnActorSpawned();
            AutoResize();
        }
    }
}
