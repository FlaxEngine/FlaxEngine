// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor
{
    /// <summary>
    /// Control that can optionally disable inputs to the children.
    /// </summary>
    [HideInEditor]
    internal class InputsPassThrough : ContainerControl
    {
        private bool _isMouseOver;

        /// <summary>
        /// True if enable input events passing to the UI.
        /// </summary>
        public virtual bool EnableInputs => true;

        public override bool RayCast(ref Float2 location, out Control hit)
        {
            return RayCastChildren(ref location, out hit);
        }

        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise)
                return false;
            return base.ContainsPoint(ref location, precise);
        }

        public override bool OnCharInput(char c)
        {
            if (!EnableInputs)
                return false;
            return base.OnCharInput(c);
        }

        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            if (!EnableInputs)
                return DragDropEffect.None;
            return base.OnDragDrop(ref location, data);
        }

        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            if (!EnableInputs)
                return DragDropEffect.None;
            return base.OnDragEnter(ref location, data);
        }

        public override void OnDragLeave()
        {
            if (!EnableInputs)
                return;
            base.OnDragLeave();
        }

        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            if (!EnableInputs)
                return DragDropEffect.None;
            return base.OnDragMove(ref location, data);
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (!EnableInputs)
                return false;
            return base.OnKeyDown(key);
        }

        public override void OnKeyUp(KeyboardKeys key)
        {
            if (!EnableInputs)
                return;
            base.OnKeyUp(key);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (!EnableInputs)
                return false;
            return base.OnMouseDoubleClick(location, button);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (!EnableInputs)
                return false;
            return base.OnMouseDown(location, button);
        }

        public override bool IsMouseOver => _isMouseOver;

        public override void OnMouseEnter(Float2 location)
        {
            _isMouseOver = true;
            if (!EnableInputs)
                return;
            base.OnMouseEnter(location);
        }

        public override void OnMouseLeave()
        {
            _isMouseOver = false;
            if (!EnableInputs)
                return;
            base.OnMouseLeave();
        }

        public override void OnMouseMove(Float2 location)
        {
            if (!EnableInputs)
                return;
            base.OnMouseMove(location);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (!EnableInputs)
                return false;
            return base.OnMouseUp(location, button);
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (!EnableInputs)
                return false;
            return base.OnMouseWheel(location, delta);
        }
    }
}
