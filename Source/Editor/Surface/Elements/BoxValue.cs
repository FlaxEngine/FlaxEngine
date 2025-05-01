// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using RealValueBox = FlaxEditor.GUI.Input.DoubleValueBox;
#else
using RealValueBox = FlaxEditor.GUI.Input.FloatValueBox;
#endif

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// Bounding Box value editing element.
    /// </summary>
    /// <seealso cref="ContainerControl" />
    /// <seealso cref="ISurfaceNodeElement" />
    [HideInEditor]
    public sealed class BoxValue : ContainerControl, ISurfaceNodeElement
    {
        private RealValueBox _minX, _minY, _minZ, _maxX, _maxY, _maxZ;
        private const float BoxWidth = 50;
        private const float LabelWidth = 26;
        private const float MarginX = 2;
        private const float MarginY = 2;
        private const float SlideSpeed = 0.1f;

        /// <inheritdoc />
        public SurfaceNode ParentNode { get; }

        /// <inheritdoc />
        public NodeElementArchetype Archetype { get; }

        /// <summary>
        /// Gets the surface.
        /// </summary>
        public VisjectSurface Surface => ParentNode.Surface;

        /// <summary>
        /// Gets the value.
        /// </summary>
        public BoundingBox Value
        {
            get => new BoundingBox(new Vector3(_minX.Value, _minY.Value, _minZ.Value), new Vector3(_maxX.Value, _maxY.Value, _maxZ.Value));
            set
            {
                _minX.Value = value.Minimum.X;
                _minY.Value = value.Minimum.Y;
                _minZ.Value = value.Minimum.Z;
                _maxX.Value = value.Maximum.X;
                _maxY.Value = value.Maximum.Y;
                _maxZ.Value = value.Maximum.Z;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="BoxValue"/> class.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="archetype">The node element archetype.</param>
        public BoxValue(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(archetype.Position.X, archetype.Position.Y, LabelWidth + (BoxWidth + MarginX) * 3, TextBox.DefaultHeight * 2 + MarginY)
        {
            ParentNode = parentNode;
            Archetype = archetype;

            var back = Style.Current.TextBoxBackground;
            var grayOutFactor = 0.6f;
            var axisColorX = CustomEditors.Editors.ActorTransformEditor.AxisColorX;
            var axisColorY = CustomEditors.Editors.ActorTransformEditor.AxisColorY;
            var axisColorZ = CustomEditors.Editors.ActorTransformEditor.AxisColorZ;

            var value = Get(parentNode, archetype);

            float minX = 0;
            float minY = 0;
            var labelMin = new Label(minX, minY, LabelWidth, TextBox.DefaultHeight)
            {
                Text = "Min",
                HorizontalAlignment = TextAlignment.Near,
                Parent = this
            };
            _minX = new RealValueBox(value.Minimum.X, LabelWidth, minY, BoxWidth, float.MinValue, float.MaxValue, SlideSpeed)
            {
                BorderColor = Color.Lerp(axisColorX, back, grayOutFactor),
                BorderSelectedColor = axisColorX,
                Parent = this
            };
            _minX.ValueChanged += OnValueChanged;
            _minY = new RealValueBox(value.Minimum.Y, _minX.Right + MarginX, minY, BoxWidth, float.MinValue, float.MaxValue, SlideSpeed)
            {
                BorderColor = Color.Lerp(axisColorY, back, grayOutFactor),
                BorderSelectedColor = axisColorY,
                Parent = this
            };
            _minY.ValueChanged += OnValueChanged;
            _minZ = new RealValueBox(value.Minimum.Z, _minY.Right + MarginX, minY, BoxWidth, float.MinValue, float.MaxValue, SlideSpeed)
            {
                BorderColor = Color.Lerp(axisColorZ, back, grayOutFactor),
                BorderSelectedColor = axisColorZ,
                Parent = this
            };
            _minZ.ValueChanged += OnValueChanged;

            float maxX = 0;
            float maxY = labelMin.Bottom + MarginY;
            var labelMax = new Label(maxX, maxY, LabelWidth, TextBox.DefaultHeight)
            {
                Text = "Max",
                HorizontalAlignment = TextAlignment.Near,
                Parent = this
            };
            _maxX = new RealValueBox(value.Maximum.X, LabelWidth, maxY, BoxWidth, float.MinValue, float.MaxValue, SlideSpeed)
            {
                BorderColor = Color.Lerp(axisColorX, back, grayOutFactor),
                BorderSelectedColor = axisColorX,
                Parent = this
            };
            _maxX.ValueChanged += OnValueChanged;
            _maxY = new RealValueBox(value.Maximum.Y, _maxX.Right + MarginX, maxY, BoxWidth, float.MinValue, float.MaxValue, SlideSpeed)
            {
                BorderColor = Color.Lerp(axisColorY, back, grayOutFactor),
                BorderSelectedColor = axisColorY,
                Parent = this
            };
            _maxY.ValueChanged += OnValueChanged;
            _maxZ = new RealValueBox(value.Maximum.Z, _maxY.Right + MarginX, maxY, BoxWidth, float.MinValue, float.MaxValue, SlideSpeed)
            {
                BorderColor = Color.Lerp(axisColorZ, back, grayOutFactor),
                BorderSelectedColor = axisColorZ,
                Parent = this
            };
            _maxZ.ValueChanged += OnValueChanged;

            ParentNode.ValuesChanged += OnNodeValuesChanged;
        }

        private void OnNodeValuesChanged()
        {
            Value = Get(ParentNode, Archetype);
        }

        private void OnValueChanged()
        {
            var value = Value;
            Set(ParentNode, Archetype, ref value);
        }

        /// <summary>
        /// Gets the floating point value from the specified parent node. Handles type casting and components gather.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="arch">The node element archetype.</param>
        /// <returns>The result value.</returns>
        public static BoundingBox Get(SurfaceNode parentNode, NodeElementArchetype arch)
        {
            if (arch.ValueIndex < 0)
                return BoundingBox.Empty;

            return (BoundingBox)parentNode.Values[arch.ValueIndex];
        }

        /// <summary>
        /// Sets the floating point value of the specified parent node. Handles type casting and components assignment.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="arch">The node element archetype.</param>
        /// <param name="toSet">The value to set.</param>
        public static void Set(SurfaceNode parentNode, NodeElementArchetype arch, ref BoundingBox toSet)
        {
            if (arch.ValueIndex < 0)
                return;

            parentNode.SetValue(arch.ValueIndex, toSet);
        }
    }
}
