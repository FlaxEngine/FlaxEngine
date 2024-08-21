// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Quaternion value type properties.
    /// </summary>
    [CustomEditor(typeof(Quaternion)), DefaultEditor]
    public class QuaternionEditor : CustomEditor
    {
        private Float3 _cachedAngles = Float3.Zero;
        private object _cachedToken;

        /// <summary>
        /// The X component element
        /// </summary>
        protected FloatValueElement XElement;

        /// <summary>
        /// The Y component element
        /// </summary>
        protected FloatValueElement YElement;

        /// <summary>
        /// The Z component element
        /// </summary>
        protected FloatValueElement ZElement;

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

            XElement = grid.FloatValue();
            XElement.ValueBox.Category = Utils.ValueCategory.Angle;
            XElement.ValueBox.ValueChanged += OnXValueChanged;
            XElement.ValueBox.SlidingEnd += () =>
            {
                _slidingEnded = true;
                ClearToken();
            };
            _defaultSlidingSpeed = XElement.ValueBox.SlideSpeed;

            YElement = grid.FloatValue();
            YElement.ValueBox.Category = Utils.ValueCategory.Angle;
            YElement.ValueBox.ValueChanged += OnYValueChanged;
            YElement.ValueBox.SlidingEnd += () =>
            {
                _slidingEnded = true;
                ClearToken();
            };

            ZElement = grid.FloatValue();
            ZElement.ValueBox.Category = Utils.ValueCategory.Angle;
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
                    var value = ((Quaternion)Values[0]).EulerAngles;
                    menu.AddButton("Copy Euler", () => { Clipboard.Text = JsonSerializer.Serialize(value); }).TooltipText = "Copy the Euler Angles in Degrees";
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
            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding;
            var token = isSliding ? this : null;
            var useCachedAngles = isSliding && token == _cachedToken;
            
            if (HasDifferentValues && Values.Count > 1)
            {
                var xValue = XElement.ValueBox.Value;
                var yValue = YElement.ValueBox.Value;
                var zValue = ZElement.ValueBox.Value;

                xValue = Mathf.UnwindDegrees(xValue);
                yValue = Mathf.UnwindDegrees(yValue);
                zValue = Mathf.UnwindDegrees(zValue);

                var value = new Float3(xValue, yValue, zValue);

                _cachedToken = token;

                var newObjects = new object[Values.Count];
                // Handle Sliding
                if (AllowSlidingForDifferentValues && (isSliding || _slidingEnded))
                {
                    Float3 average = Float3.Zero;
                    for (int i = 0; i < Values.Count; i++)
                    {
                        var v = (Quaternion)Values[0];
                        var euler = v.EulerAngles;
                    
                        average += euler;
                    }
                    
                    average /= Values.Count;

                    var newValue = value - average;

                    for (int i = 0; i < Values.Count; i++)
                    {
                        var v = Values[i];
                        var val = (Quaternion)v;
                        Quaternion.Euler(_valueChanged == ValueChanged.X ? newValue.X : 0, _valueChanged == ValueChanged.Y ? newValue.Y : 0, _valueChanged == ValueChanged.Z ? newValue.Z : 0, out Quaternion qVal);
                        v = val * qVal;

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
                        var val = (Quaternion)v;
                        var euler = val.EulerAngles;
                        Quaternion.Euler(_valueChanged == ValueChanged.X ? xValue : euler.X, _valueChanged == ValueChanged.Y ? yValue : euler.Y, _valueChanged == ValueChanged.Z ? zValue : euler.Z, out Quaternion qVal);
                        v = val * qVal;

                        newObjects[i] = v;
                    }
                }
                
                SetValue(newObjects, token);
            }
            else
            {
                float x = (useCachedAngles && !XElement.IsSliding) ? _cachedAngles.X : XElement.ValueBox.Value;
                float y = (useCachedAngles && !YElement.IsSliding) ? _cachedAngles.Y : YElement.ValueBox.Value;
                float z = (useCachedAngles && !ZElement.IsSliding) ? _cachedAngles.Z : ZElement.ValueBox.Value;

                x = Mathf.UnwindDegrees(x);
                y = Mathf.UnwindDegrees(y);
                z = Mathf.UnwindDegrees(z);

                if (!useCachedAngles)
                {
                    _cachedAngles = new Float3(x, y, z);
                }

                _cachedToken = token;

                Quaternion.Euler(x, y, z, out Quaternion value);
                SetValue(value, token);
            }
        }

        /// <inheritdoc />
        protected override void ClearToken()
        {
            _cachedToken = null;
            base.ClearToken();
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
                    var value = (Quaternion)Values[i];
                    var euler = value.EulerAngles;
                    
                    average += euler;
                    
                    if (i == 0)
                    {
                        cachedFirstValue = euler;
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
                var value = (Quaternion)Values[0];
                var euler = value.EulerAngles;
                XElement.ValueBox.Value = euler.X;
                YElement.ValueBox.Value = euler.Y;
                ZElement.ValueBox.Value = euler.Z;
                
                if (!Mathf.NearEqual(XElement.ValueBox.SlideSpeed, _defaultSlidingSpeed))
                    XElement.ValueBox.SlideSpeed = _defaultSlidingSpeed;
                if (!Mathf.NearEqual(YElement.ValueBox.SlideSpeed, _defaultSlidingSpeed))
                    YElement.ValueBox.SlideSpeed = _defaultSlidingSpeed;
                if (!Mathf.NearEqual(ZElement.ValueBox.SlideSpeed, _defaultSlidingSpeed))
                    ZElement.ValueBox.SlideSpeed = _defaultSlidingSpeed;
            }
        }
    }
}
