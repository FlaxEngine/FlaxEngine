// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.IO;
using System.Text;
using FlaxEngine.GUI;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;

namespace FlaxEngine
{
    partial class UIControl
    {
        private Control _control;
        private static bool _blockEvents; // Used to ignore internal events from C++ UIControl impl when performing state sync with C# UI

        /// <summary>
        /// Gets or sets the GUI control used by this actor.
        /// </summary>
        /// <remarks>
        /// When changing the control, the previous one is disposed. Use <see cref="UnlinkControl"/> to manage it on your own.
        /// </remarks>
        [EditorDisplay("Control", EditorDisplayAttribute.InlineStyle), CustomEditorAlias("FlaxEditor.CustomEditors.Dedicated.UIControlControlEditor"), EditorOrder(50)]
        public Control Control
        {
            get => _control;
            set
            {
                if (_control == value)
                    return;

                // Cleanup previous
                var prevControl = _control;
                if (prevControl != null)
                {
                    prevControl.LocationChanged -= OnControlLocationChanged;
                    prevControl.Dispose();
                }

                // Set value
                _control = value;

                // Link the new one (events and parent)
                if (_control != null)
                {
                    // Setup control
                    _blockEvents = true;
                    var containerControl = _control as ContainerControl;
                    if (containerControl != null)
                        containerControl.UnlockChildrenRecursive();
                    _control.Parent = GetParent();
                    _control.IndexInParent = OrderInParent;
                    _control.Location = new Vector2(LocalPosition);
                    // TODO: sync control order in parent with actor order in parent (think about special cases like Panel with scroll bars used as internal controls)
                    _control.LocationChanged += OnControlLocationChanged;

                    // Link children UI controls
                    if (containerControl != null && IsActiveInHierarchy)
                    {
                        var children = ChildrenCount;
                        for (int i = 0; i < children; i++)
                        {
                            var child = GetChild(i) as UIControl;
                            if (child != null && child.IsActiveInHierarchy && child.HasControl)
                            {
                                child.Control.Parent = containerControl;
                            }
                        }
                    }

                    // Refresh
                    _blockEvents = false;
                    if (prevControl == null && _control.Parent != null)
                        _control.Parent.PerformLayout();
                    else
                        _control.PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether this actor has control.
        /// </summary>
        public bool HasControl => _control != null;

        /// <summary>
        /// Gets the world-space oriented bounding box that contains a 3D control.
        /// </summary>
        public OrientedBoundingBox Bounds
        {
            get
            {
                // Pick a parent canvas
                var canvasRoot = Control?.Root as CanvasRootControl;
                if (canvasRoot == null || canvasRoot.Canvas.Is2D)
                    return new OrientedBoundingBox();

                // Find control bounds limit points in canvas-space
                Vector2 p1 = Vector2.Zero;
                Vector2 p2 = new Vector2(0, Control.Height);
                Vector2 p3 = new Vector2(Control.Width, 0);
                Vector2 p4 = Control.Size;
                Control c = Control;
                while (c != canvasRoot)
                {
                    p1 = c.PointToParent(ref p1);
                    p2 = c.PointToParent(ref p2);
                    p3 = c.PointToParent(ref p3);
                    p4 = c.PointToParent(ref p4);

                    c = c.Parent;
                }
                Vector2 min = Vector2.Min(Vector2.Min(p1, p2), Vector2.Min(p3, p4));
                Vector2 max = Vector2.Max(Vector2.Max(p1, p2), Vector2.Max(p3, p4));
                Vector2 size = max - min;

                // Calculate bounds
                OrientedBoundingBox bounds = new OrientedBoundingBox
                {
                    Extents = new Vector3(size * 0.5f, Mathf.Epsilon)
                };

                canvasRoot.Canvas.GetWorldMatrix(out Matrix world);
                Matrix.Translation(min.X + size.X * 0.5f, min.Y + size.Y * 0.5f, 0, out Matrix offset);
                Matrix.Multiply(ref offset, ref world, out bounds.Transformation);
                return bounds;
            }
        }

        /// <summary>
        /// Gets the control object cased to the given type.
        /// </summary>
        /// <typeparam name="T">The type of the control.</typeparam>
        /// <returns>The control object.</returns>
        public T Get<T>() where T : Control
        {
            return (T)_control;
        }

        /// <summary>
        /// Checks if the control object is of the given type.
        /// </summary>
        /// <typeparam name="T">The type of the control.</typeparam>
        /// <returns>True if control object is of the given type.</returns>
        public bool Is<T>() where T : Control
        {
            return _control is T;
        }

        /// <summary>
        /// Creates a new UIControl with the control of the given type and links it to this control as a child.
        /// </summary>
        /// <remarks>
        /// The current actor has to have a valid container control.
        /// </remarks>
        /// <typeparam name="T">Type of the child control to add.</typeparam>
        /// <returns>The created UIControl that contains a new control of the given type.</returns>
        public UIControl AddChildControl<T>() where T : Control
        {
            if (!(_control is ContainerControl))
                throw new InvalidOperationException("To add child to the control it has to be ContainerControl.");

            var child = new UIControl
            {
                Parent = this,
                Control = (Control)Activator.CreateInstance(typeof(T))
            };
            return child;
        }

        /// <summary>
        /// Unlinks the control from the actor without disposing it or modifying.
        /// </summary>
        [NoAnimate]
        public void UnlinkControl()
        {
            if (_control != null)
            {
                _control.LocationChanged -= OnControlLocationChanged;
                _control = null;
            }
        }

        private void OnControlLocationChanged(Control control)
        {
            _blockEvents = true;
            LocalPosition = new Vector3(control.Location, LocalPosition.Z);
            _blockEvents = false;
        }

        /// <summary>
        /// The fallback callback used to handle <see cref="UIControl"/> parent container control to link when it fails to find the default parent. Can be used to link the controls into a custom control.
        /// </summary>
        public static Func<UIControl, ContainerControl> FallbackParentGetDelegate;

        private ContainerControl GetParent()
        {
            // Don't link disabled actors
            if (!IsActiveInHierarchy)
                return null;

            var parent = Parent;
            if (parent is UIControl uiControl && uiControl.Control is ContainerControl uiContainerControl)
                return uiContainerControl;
            if (parent is UICanvas uiCanvas)
                return uiCanvas.GUI;
            return FallbackParentGetDelegate?.Invoke(this);
        }

        internal string Serialize(out string controlType)
        {
            if (_control == null)
            {
                controlType = null;
                return null;
            }

            var type = _control.GetType();

            JsonSerializer jsonSerializer = JsonSerializer.CreateDefault(Json.JsonSerializer.Settings);
            jsonSerializer.Formatting = Formatting.Indented;

            StringBuilder sb = new StringBuilder(1024);
            StringWriter sw = new StringWriter(sb, CultureInfo.InvariantCulture);
            using (JsonTextWriter jsonWriter = new JsonTextWriter(sw))
            {
                // Prepare writer settings
                jsonWriter.IndentChar = '\t';
                jsonWriter.Indentation = 1;
                jsonWriter.Formatting = jsonSerializer.Formatting;
                jsonWriter.DateFormatHandling = jsonSerializer.DateFormatHandling;
                jsonWriter.DateTimeZoneHandling = jsonSerializer.DateTimeZoneHandling;
                jsonWriter.FloatFormatHandling = jsonSerializer.FloatFormatHandling;
                jsonWriter.StringEscapeHandling = jsonSerializer.StringEscapeHandling;
                jsonWriter.Culture = jsonSerializer.Culture;
                jsonWriter.DateFormatString = jsonSerializer.DateFormatString;

                JsonSerializerInternalWriter serializerWriter = new JsonSerializerInternalWriter(jsonSerializer);

                serializerWriter.Serialize(jsonWriter, _control, type);
            }

            controlType = type.FullName;
            return sw.ToString();
        }

        internal string SerializeDiff(out string controlType, UIControl other)
        {
            if (_control == null)
            {
                controlType = null;
                return null;
            }
            var type = _control.GetType();
            if (other._control == null || other._control.GetType() != type)
            {
                return Serialize(out controlType);
            }

            JsonSerializer jsonSerializer = JsonSerializer.CreateDefault(Json.JsonSerializer.Settings);
            jsonSerializer.Formatting = Formatting.Indented;

            StringBuilder sb = new StringBuilder(1024);
            StringWriter sw = new StringWriter(sb, CultureInfo.InvariantCulture);
            using (JsonTextWriter jsonWriter = new JsonTextWriter(sw))
            {
                // Prepare writer settings
                jsonWriter.IndentChar = '\t';
                jsonWriter.Indentation = 1;
                jsonWriter.Formatting = jsonSerializer.Formatting;
                jsonWriter.DateFormatHandling = jsonSerializer.DateFormatHandling;
                jsonWriter.DateTimeZoneHandling = jsonSerializer.DateTimeZoneHandling;
                jsonWriter.FloatFormatHandling = jsonSerializer.FloatFormatHandling;
                jsonWriter.StringEscapeHandling = jsonSerializer.StringEscapeHandling;
                jsonWriter.Culture = jsonSerializer.Culture;
                jsonWriter.DateFormatString = jsonSerializer.DateFormatString;

                JsonSerializerInternalWriter serializerWriter = new JsonSerializerInternalWriter(jsonSerializer);

                serializerWriter.SerializeDiff(jsonWriter, _control, type, other._control);
            }

            controlType = string.Empty;
            return sw.ToString();
        }

        internal void Deserialize(string json, Type controlType)
        {
            if (_control == null || _control.GetType() != controlType)
            {
                if (controlType != null)
                    Control = (Control)Activator.CreateInstance(controlType);
            }

            if (_control != null)
            {
                Json.JsonSerializer.Deserialize(_control, json);
            }
        }

        internal void ParentChanged()
        {
            if (_control != null && !_blockEvents)
            {
                _control.Parent = GetParent();
                _control.IndexInParent = OrderInParent;
            }
        }

        internal void TransformChanged()
        {
            if (_control != null && !_blockEvents)
            {
                _control.Location = new Vector2(LocalPosition);
            }
        }

        internal void ActiveInTreeChanged()
        {
            if (_control != null && !_blockEvents)
            {
                // Link or unlink control (won't modify Enable/Visible state)
                _control.Parent = GetParent();
                _control.IndexInParent = OrderInParent;
            }
        }

        internal void OrderInParentChanged()
        {
            if (_control != null && !_blockEvents)
            {
                _control.IndexInParent = OrderInParent;
            }
        }

        internal void BeginPlay()
        {
            if (_control != null)
            {
                _control.Parent = GetParent();
            }
        }

        internal void EndPlay()
        {
            if (_control != null)
            {
                _control.Dispose();
                _control = null;
            }
        }
    }
}
