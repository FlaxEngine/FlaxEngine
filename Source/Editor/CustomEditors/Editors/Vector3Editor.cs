// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// The value changed by custom Vector3 editors.
    /// </summary>
    public enum ValueChanged
    {
        /// <summary>
        /// X value changed.
        /// </summary>
        X = 0,

        /// <summary>
        /// Y value changed.
        /// </summary>
        Y = 1,

        /// <summary>
        /// Z value changed.
        /// </summary>
        Z = 2
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Vector3 value type properties.
    /// </summary>
    [CustomEditor(typeof(Vector3)), DefaultEditor]
    public class Vector3Editor :
#if USE_LARGE_WORLDS
    Double3Editor
#else
    Float3Editor
#endif
    {
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Float3 value type properties.
    /// </summary>
    [CustomEditor(typeof(Float3)), DefaultEditor]
    public class Float3Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected FloatValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected FloatValueElement YElement;

        /// <summary>
        /// The Z component editor.
        /// </summary>
        protected FloatValueElement ZElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <summary>
        /// If true, when one value is changed, the other 2 will change as well.
        /// </summary>
        public bool LinkValues = false;

        /// <summary>
        /// Whether to use the average for sliding different values.
        /// </summary>
        public virtual bool AllowSlidingForDifferentValues => true;

        private ValueChanged _valueChanged;
        private float _defaultSlidingSpeed;
        private bool _slidingEnded = false;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<UniformGridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 3;
            gridControl.SlotsVertically = 1;

            LimitAttribute limit = null;
            var attributes = Values.GetAttributes();
            var category = Utils.ValueCategory.None;
            if (attributes != null)
            {
                limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
                var categoryAttribute = (ValueCategoryAttribute)attributes.FirstOrDefault(x => x is ValueCategoryAttribute);
                if (categoryAttribute != null)
                    category = categoryAttribute.Category;
            }

            XElement = grid.FloatValue();
            XElement.SetLimits(limit);
            XElement.SetCategory(category);
            XElement.ValueBox.ValueChanged += OnXValueChanged;
            XElement.ValueBox.SlidingEnd += () =>
            {
                _slidingEnded = true;
                ClearToken();
            };
            _defaultSlidingSpeed = XElement.ValueBox.SlideSpeed;

            YElement = grid.FloatValue();
            YElement.SetLimits(limit);
            YElement.SetCategory(category);
            YElement.ValueBox.ValueChanged += OnYValueChanged;
            YElement.ValueBox.SlidingEnd += () =>
            {
                _slidingEnded = true;
                ClearToken();
            };

            ZElement = grid.FloatValue();
            ZElement.SetLimits(limit);
            ZElement.SetCategory(category);
            ZElement.ValueBox.ValueChanged += OnZValueChanged;
            ZElement.ValueBox.SlidingEnd += () =>
            {
                _slidingEnded = true;
                ClearToken();
            };

            if (LinkedLabel != null)
            {
                LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                {
                    menu.AddSeparator();
                    var mb = menu.AddButton("Show formatted", bt =>
                    {
                        XElement.SetCategory(bt.Checked ? category : Utils.ValueCategory.None);
                        YElement.SetCategory(bt.Checked ? category : Utils.ValueCategory.None);
                        ZElement.SetCategory(bt.Checked ? category : Utils.ValueCategory.None);
                    });
                    mb.AutoCheck = true;
                    mb.Checked = XElement.ValueBox.Category != Utils.ValueCategory.None;
                };
            }
        }

        private void OnXValueChanged()
        {
            if (IsSetBlocked)
                return;
            _valueChanged = ValueChanged.X;
            OnValueChanged();
        }

        private void OnYValueChanged()
        {
            if (IsSetBlocked)
                return;
            _valueChanged = ValueChanged.Y;
            OnValueChanged();
        }

        private void OnZValueChanged()
        {
            if (IsSetBlocked)
                return;
            _valueChanged = ValueChanged.Z;
            OnValueChanged();
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked || Values == null)
                return;

            var xValue = XElement.ValueBox.Value;
            var yValue = YElement.ValueBox.Value;
            var zValue = ZElement.ValueBox.Value;

            if (LinkValues)
            {
                var valueRatio = 0.0f;
                switch (_valueChanged)
                {
                case ValueChanged.X:
                    valueRatio = GetRatio(xValue, ((Float3)Values[0]).X);
                    if (Mathf.NearEqual(valueRatio, 0))
                    {
                        XElement.ValueBox.Enabled = false;
                        valueRatio = 1;
                    }
                    yValue = NewLinkedValue(yValue, valueRatio);
                    zValue = NewLinkedValue(zValue, valueRatio);
                    break;
                case ValueChanged.Y:
                    valueRatio = GetRatio(yValue, ((Float3)Values[0]).Y);
                    if (Mathf.NearEqual(valueRatio, 0))
                    {
                        YElement.ValueBox.Enabled = false;
                        valueRatio = 1;
                    }
                    xValue = NewLinkedValue(xValue, valueRatio);
                    zValue = NewLinkedValue(zValue, valueRatio);
                    break;
                case ValueChanged.Z:
                    valueRatio = GetRatio(zValue, ((Float3)Values[0]).Z);
                    if (Mathf.NearEqual(valueRatio, 0))
                    {
                        ZElement.ValueBox.Enabled = false;
                        valueRatio = 1;
                    }
                    xValue = NewLinkedValue(xValue, valueRatio);
                    yValue = NewLinkedValue(yValue, valueRatio);
                    break;
                }
            }

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Float3(xValue, yValue, zValue);
            if (HasDifferentValues && Values.Count > 1)
            {
                var newObjects = new object[Values.Count];
                // Handle Sliding
                if (AllowSlidingForDifferentValues && (isSliding || _slidingEnded))
                {
                    Float3 average = Float3.Zero;
                    for (int i = 0; i < Values.Count; i++)
                    {
                        var v = Values[i];
                        var castedValue = Float3.Zero;
                        if (v is Vector3 asVector3)
                            castedValue = asVector3;
                        else if (v is Float3 asFloat3)
                            castedValue = asFloat3;
                        else if (v is Double3 asDouble3)
                            castedValue = asDouble3;

                        average += castedValue;
                    }
                    
                    average /= Values.Count;

                    var newValue = value - average;

                    for (int i = 0; i < Values.Count; i++)
                    {
                        var v = Values[i];
                        if (LinkValues)
                        {
                            if (v is Vector3 asVector3)
                                v = asVector3 + new Vector3(newValue.X, newValue.Y, newValue.Z);
                            else if (v is Float3 asFloat3)
                                v = asFloat3 + new Float3(newValue.X, newValue.Y, newValue.Z);
                            else if (v is Double3 asDouble3)
                                v = asDouble3 + new Double3(newValue.X, newValue.Y, newValue.Z);
                        }
                        else
                        {
                            if (v is Vector3 asVector3)
                                v = asVector3 + new Vector3(_valueChanged == ValueChanged.X ? newValue.X : 0, _valueChanged == ValueChanged.Y ? newValue.Y : 0, _valueChanged == ValueChanged.Z ? newValue.Z : 0);
                            else if (v is Float3 asFloat3)
                                v = asFloat3 + new Float3(_valueChanged == ValueChanged.X ? newValue.X : 0, _valueChanged == ValueChanged.Y ? newValue.Y : 0, _valueChanged == ValueChanged.Z ? newValue.Z : 0);
                            else if (v is Double3 asDouble3)
                                v = asDouble3 + new Double3(_valueChanged == ValueChanged.X ? newValue.X : 0, _valueChanged == ValueChanged.Y ? newValue.Y : 0, _valueChanged == ValueChanged.Z ? newValue.Z : 0);
                        }

                        newObjects[i] = v;
                    }

                    // Capture last sliding value
                    if (_slidingEnded)
                        _slidingEnded = false;
                }
                else
                {
                    for (int i = 0; i < Values.Count; i++)
                    {
                        object v = Values[i];
                        if (LinkValues)
                        {
                            if (v is Vector3 asVector3)
                                v = asVector3 + new Vector3(xValue, yValue, zValue);
                            else if (v is Float3 asFloat3)
                                v = asFloat3 + new Float3(xValue, yValue, zValue);
                            else if (v is Double3 asDouble3)
                                v = asDouble3 + new Double3(xValue, yValue, zValue);
                        }
                        else
                        {
                            if (v is Vector3 asVector3)
                                v = new Vector3(_valueChanged == ValueChanged.X ? xValue : asVector3.X, _valueChanged == ValueChanged.Y ? yValue : asVector3.Y, _valueChanged == ValueChanged.Z ? zValue : asVector3.Z);
                            else if (v is Float3 asFloat3)
                                v = new Float3(_valueChanged == ValueChanged.X ? xValue : asFloat3.X, _valueChanged == ValueChanged.Y ? yValue : asFloat3.Y, _valueChanged == ValueChanged.Z ? zValue : asFloat3.Z);
                            else if (v is Double3 asDouble3)
                                v = new Double3(_valueChanged == ValueChanged.X ? xValue : asDouble3.X, _valueChanged == ValueChanged.Y ? yValue : asDouble3.Y, _valueChanged == ValueChanged.Z ? zValue : asDouble3.Z);
                        }

                        newObjects[i] = v;
                    }
                }
                
                SetValue(newObjects, token);
            }
            else
            {
                object v = Values[0];
                if (v is Vector3)
                    v = (Vector3)value;
                else if (v is Float3)
                    v = (Float3)value;
                else if (v is Double3)
                    v = (Double3)value;
                SetValue(v, token);
            }
        }

        private float GetRatio(float value, float initialValue)
        {
            return Mathf.NearEqual(initialValue, 0) ? 0 : value / initialValue;
        }

        private float NewLinkedValue(float value, float valueRatio)
        {
            return Mathf.NearEqual(value, 0) ? value : value * valueRatio;
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // Get which values are different
                bool xDifferent = false;
                bool yDifferent = false;
                bool zDifferent = false;
                Float3 cachedFirstValue = Float3.Zero;
                Float3 average = Float3.Zero;
                for (int i = 0; i < Values.Count; i++)
                {
                    var v = Values[i];
                    var value = Float3.Zero;
                    if (v is Vector3 asVector3)
                        value = asVector3;
                    else if (v is Float3 asFloat3)
                        value = asFloat3;
                    else if (v is Double3 asDouble3)
                        value = asDouble3;
                    
                    average += value;
                    
                    if (i == 0)
                    {
                        cachedFirstValue = value;
                        continue;
                    }

                    if (!Mathf.NearEqual(cachedFirstValue.X, value.X))
                        xDifferent = true;
                    if (!Mathf.NearEqual(cachedFirstValue.Y, value.Y))
                        yDifferent = true;
                    if (!Mathf.NearEqual(cachedFirstValue.Z, value.Z))
                        zDifferent = true;
                }
                
                average /= Values.Count;
                
                if (!xDifferent)
                {
                    XElement.ValueBox.Value = cachedFirstValue.X;
                }
                else
                {
                    if (AllowSlidingForDifferentValues)
                        XElement.ValueBox.Value = average.X;
                    else
                        XElement.ValueBox.SlideSpeed = 0;
                    XElement.ValueBox.Text = "---";
                }
                
                if (!yDifferent)
                {
                    YElement.ValueBox.Value = cachedFirstValue.Y;
                }
                else
                {
                    if (AllowSlidingForDifferentValues)
                        YElement.ValueBox.Value = average.Y;
                    else
                        YElement.ValueBox.SlideSpeed = 0;
                    YElement.ValueBox.Text = "---";
                }
                
                if (!zDifferent)
                {
                    ZElement.ValueBox.Value = cachedFirstValue.Z;
                }
                else
                {
                    if (AllowSlidingForDifferentValues)
                        ZElement.ValueBox.Value = average.Z;
                    else
                        ZElement.ValueBox.SlideSpeed = 0;
                    ZElement.ValueBox.Text = "---";
                }
            }
            else
            {
                var value = Float3.Zero;
                if (Values[0] is Vector3 asVector3)
                    value = asVector3;
                else if (Values[0] is Float3 asFloat3)
                    value = asFloat3;
                else if (Values[0] is Double3 asDouble3)
                    value = asDouble3;
                XElement.ValueBox.Value = value.X;
                YElement.ValueBox.Value = value.Y;
                ZElement.ValueBox.Value = value.Z;

                if (!Mathf.NearEqual(XElement.ValueBox.SlideSpeed, _defaultSlidingSpeed))
                    XElement.ValueBox.SlideSpeed = _defaultSlidingSpeed;
                if (!Mathf.NearEqual(YElement.ValueBox.SlideSpeed, _defaultSlidingSpeed))
                    YElement.ValueBox.SlideSpeed = _defaultSlidingSpeed;
                if (!Mathf.NearEqual(ZElement.ValueBox.SlideSpeed, _defaultSlidingSpeed))
                    ZElement.ValueBox.SlideSpeed = _defaultSlidingSpeed;
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Double3 value type properties.
    /// </summary>
    [CustomEditor(typeof(Double3)), DefaultEditor]
    public class Double3Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected DoubleValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected DoubleValueElement YElement;

        /// <summary>
        /// The Z component editor.
        /// </summary>
        protected DoubleValueElement ZElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;
        
        /// <summary>
        /// Whether to use the average for sliding different values.
        /// </summary>
        public virtual bool AllowSlidingForDifferentValues => true;

        private ValueChanged _valueChanged;
        private float _defaultSlidingSpeed;
        private bool _slidingEnded = false;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<UniformGridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 3;
            gridControl.SlotsVertically = 1;

            LimitAttribute limit = null;
            Utils.ValueCategory category = Utils.ValueCategory.None;
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
                var categoryAttribute = (ValueCategoryAttribute)attributes.FirstOrDefault(x => x is ValueCategoryAttribute);
                if (categoryAttribute != null)
                    category = categoryAttribute.Category;
            }

            XElement = grid.DoubleValue();
            XElement.SetLimits(limit);
            XElement.SetCategory(category);
            XElement.ValueBox.ValueChanged += OnXValueChanged;
            XElement.ValueBox.SlidingEnd += () =>
            {
                _slidingEnded = true;
                ClearToken();
            };
            _defaultSlidingSpeed = XElement.ValueBox.SlideSpeed;

            YElement = grid.DoubleValue();
            YElement.SetLimits(limit);
            YElement.SetCategory(category);
            YElement.ValueBox.ValueChanged += OnYValueChanged;
            YElement.ValueBox.SlidingEnd += () =>
            {
                _slidingEnded = true;
                ClearToken();
            };

            ZElement = grid.DoubleValue();
            ZElement.SetLimits(limit);
            ZElement.SetCategory(category);
            ZElement.ValueBox.ValueChanged += OnZValueChanged;
            ZElement.ValueBox.SlidingEnd += () =>
            {
                _slidingEnded = true;
                ClearToken();
            };

            if (LinkedLabel != null)
            {
                LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                {
                    menu.AddSeparator();
                    var mb = menu.AddButton("Show formatted", bt =>
                    {
                        XElement.SetCategory(bt.Checked ? category : Utils.ValueCategory.None);
                        YElement.SetCategory(bt.Checked ? category : Utils.ValueCategory.None);
                        ZElement.SetCategory(bt.Checked ? category : Utils.ValueCategory.None);
                    });
                    mb.AutoCheck = true;
                    mb.Checked = XElement.ValueBox.Category != Utils.ValueCategory.None;
                };
            }
        }

              private void OnXValueChanged()
        {
            if (IsSetBlocked)
                return;
            _valueChanged = ValueChanged.X;
            OnValueChanged();
        }

        private void OnYValueChanged()
        {
            if (IsSetBlocked)
                return;
            _valueChanged = ValueChanged.Y;
            OnValueChanged();
        }

        private void OnZValueChanged()
        {
            if (IsSetBlocked)
                return;
            _valueChanged = ValueChanged.Z;
            OnValueChanged();
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked || Values == null)
                return;

            var xValue = XElement.ValueBox.Value;
            var yValue = YElement.ValueBox.Value;
            var zValue = ZElement.ValueBox.Value;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Double3(xValue, yValue, zValue);
            if (HasDifferentValues && Values.Count > 1)
            {
                var newObjects = new object[Values.Count];
                // Handle Sliding
                if (AllowSlidingForDifferentValues && (isSliding || _slidingEnded))
                {
                    // TODO: handle linked values
                    Double3 average = Double3.Zero;
                    for (int i = 0; i < Values.Count; i++)
                    {
                        var v = Values[i];
                        var castedValue = Double3.Zero;
                        if (v is Vector3 asVector3)
                            castedValue = asVector3;
                        else if (v is Float3 asFloat3)
                            castedValue = asFloat3;
                        else if (v is Double3 asDouble3)
                            castedValue = asDouble3;

                        average += castedValue;
                    }
                    
                    average /= Values.Count;

                    var newValue = value - average;

                    for (int i = 0; i < Values.Count; i++)
                    {
                        var v = Values[i];
                        if (v is Vector3 asVector3)
                            v = asVector3 + new Vector3(_valueChanged == ValueChanged.X ? newValue.X : 0, _valueChanged == ValueChanged.Y ? newValue.Y : 0, _valueChanged == ValueChanged.Z ? newValue.Z : 0);
                        else if (v is Float3 asFloat3)
                            v = asFloat3 + new Float3(_valueChanged == ValueChanged.X ? (float)newValue.X : 0, _valueChanged == ValueChanged.Y ? (float)newValue.Y : 0, _valueChanged == ValueChanged.Z ? (float)newValue.Z : 0);
                        else if (v is Double3 asDouble3)
                            v = asDouble3 + new Double3(_valueChanged == ValueChanged.X ? newValue.X : 0, _valueChanged == ValueChanged.Y ? newValue.Y : 0, _valueChanged == ValueChanged.Z ? newValue.Z : 0);
                        
                        newObjects[i] = v;
                    }

                    // Capture last sliding value
                    if (_slidingEnded)
                        _slidingEnded = false;
                }
                else
                {
                    // TODO: handle linked values
                    for (int i = 0; i < Values.Count; i++)
                    {
                        object v = Values[i];
                        if (v is Vector3 asVector3)
                            v = new Vector3(_valueChanged == ValueChanged.X ? xValue : asVector3.X, _valueChanged == ValueChanged.Y ? yValue : asVector3.Y, _valueChanged == ValueChanged.Z ? zValue : asVector3.Z);
                        else if (v is Float3 asFloat3)
                            v = new Float3(_valueChanged == ValueChanged.X ? (float)xValue : asFloat3.X, _valueChanged == ValueChanged.Y ? (float)yValue : asFloat3.Y, _valueChanged == ValueChanged.Z ? (float)zValue : asFloat3.Z);
                        else if (v is Double3 asDouble3)
                            v = new Double3(_valueChanged == ValueChanged.X ? xValue : asDouble3.X, _valueChanged == ValueChanged.Y ? yValue : asDouble3.Y, _valueChanged == ValueChanged.Z ? zValue : asDouble3.Z);
                        
                        newObjects[i] = v;
                    }
                }
                
                SetValue(newObjects, token);
            }
            else
            {
                object v = Values[0];
                if (v is Vector3)
                    v = (Vector3)value;
                else if (v is Float3)
                    v = (Float3)value;
                else if (v is Double3)
                    v = (Double3)value;
                SetValue(v, token);
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // Get which values are different
                bool xDifferent = false;
                bool yDifferent = false;
                bool zDifferent = false;
                Double3 cachedFirstValue = Double3.Zero;
                Double3 average = Double3.Zero;
                for (int i = 0; i < Values.Count; i++)
                {
                    var v = Values[i];
                    var value = Double3.Zero;
                    if (v is Vector3 asVector3)
                        value = asVector3;
                    else if (v is Float3 asFloat3)
                        value = asFloat3;
                    else if (v is Double3 asDouble3)
                        value = asDouble3;
                    
                    average += value;
                    
                    if (i == 0)
                    {
                        cachedFirstValue = value;
                        continue;
                    }

                    if (!Mathd.NearEqual(cachedFirstValue.X, value.X))
                        xDifferent = true;
                    if (!Mathd.NearEqual(cachedFirstValue.Y, value.Y))
                        yDifferent = true;
                    if (!Mathd.NearEqual(cachedFirstValue.Z, value.Z))
                        zDifferent = true;
                }
                
                average /= Values.Count;
                
                if (!xDifferent)
                {
                    XElement.ValueBox.Value = cachedFirstValue.X;
                }
                else
                {
                    if (AllowSlidingForDifferentValues)
                        XElement.ValueBox.Value = average.X;
                    else
                        XElement.ValueBox.SlideSpeed = 0;
                    XElement.ValueBox.Text = "---";
                }
                
                if (!yDifferent)
                {
                    YElement.ValueBox.Value = cachedFirstValue.Y;
                }
                else
                {
                    if (AllowSlidingForDifferentValues)
                        YElement.ValueBox.Value = average.Y;
                    else
                        YElement.ValueBox.SlideSpeed = 0;
                    YElement.ValueBox.Text = "---";
                }
                
                if (!zDifferent)
                {
                    ZElement.ValueBox.Value = cachedFirstValue.Z;
                }
                else
                {
                    if (AllowSlidingForDifferentValues)
                        ZElement.ValueBox.Value = average.Z;
                    else
                        ZElement.ValueBox.SlideSpeed = 0;
                    ZElement.ValueBox.Text = "---";
                }
            }
            else
            {
                var value = Double3.Zero;
                if (Values[0] is Vector3 asVector3)
                    value = asVector3;
                else if (Values[0] is Float3 asFloat3)
                    value = asFloat3;
                else if (Values[0] is Double3 asDouble3)
                    value = asDouble3;
                XElement.ValueBox.Value = value.X;
                YElement.ValueBox.Value = value.Y;
                ZElement.ValueBox.Value = value.Z;

                if (!Mathf.NearEqual(XElement.ValueBox.SlideSpeed, _defaultSlidingSpeed))
                {
                    XElement.ValueBox.SlideSpeed = _defaultSlidingSpeed;
                }
                if (!Mathf.NearEqual(YElement.ValueBox.SlideSpeed, _defaultSlidingSpeed))
                {
                    YElement.ValueBox.SlideSpeed = _defaultSlidingSpeed;
                }
                if (!Mathf.NearEqual(ZElement.ValueBox.SlideSpeed, _defaultSlidingSpeed))
                {
                    ZElement.ValueBox.SlideSpeed = _defaultSlidingSpeed;
                }
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Int3 value type properties.
    /// </summary>
    [CustomEditor(typeof(Int3)), DefaultEditor]
    public class Int3Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected IntegerValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected IntegerValueElement YElement;

        /// <summary>
        /// The Z component editor.
        /// </summary>
        protected IntegerValueElement ZElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<UniformGridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 3;
            gridControl.SlotsVertically = 1;

            LimitAttribute limit = null;
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
            }

            XElement = grid.IntegerValue();
            XElement.SetLimits(limit);
            XElement.IntValue.ValueChanged += OnValueChanged;
            XElement.IntValue.SlidingEnd += ClearToken;

            YElement = grid.IntegerValue();
            YElement.SetLimits(limit);
            YElement.IntValue.ValueChanged += OnValueChanged;
            YElement.IntValue.SlidingEnd += ClearToken;

            ZElement = grid.IntegerValue();
            ZElement.SetLimits(limit);
            ZElement.IntValue.ValueChanged += OnValueChanged;
            ZElement.IntValue.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked || Values == null)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Int3(XElement.IntValue.Value, YElement.IntValue.Value, ZElement.IntValue.Value);
            SetValue(value, token);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values for ValueBox<T>
            }
            else
            {
                var value = (Int3)Values[0];
                XElement.IntValue.Value = value.X;
                YElement.IntValue.Value = value.Y;
                ZElement.IntValue.Value = value.Z;
            }
        }
    }
}
