// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track for animating object member (managed object).
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public abstract class MemberTrack : ConductorTrack
    {
        private float _previewValueLeft;

        /// <summary>
        /// The member value data size (in bytes).
        /// </summary>
        public int ValueSize;

        /// <summary>
        /// Gets or sets the object member name (just a member name). Does not validate the value on set.
        /// </summary>
        public string MemberName
        {
            get => Title;
            set => Title = value;
        }

        /// <summary>
        /// The member typename (fullname including namespace but not assembly).
        /// </summary>
        public string MemberTypeName;

        /// <summary>
        /// Gets or sets the object member. Performs the value validation on set.
        /// </summary>
        public MemberInfo Member
        {
            get
            {
                if (ParentTrack is IObjectTrack objectTrack)
                {
                    var obj = objectTrack.Object;
                    if (obj != null)
                    {
                        return GetMember(obj.GetType(), MemberName);
                    }
                }
                return null;
            }
            set
            {
                if (value != null && ParentTrack is IObjectTrack objectTrack)
                {
                    var obj = objectTrack.Object;
                    if (obj != null)
                    {
                        if (GetMember(obj.GetType(), value.Name) == null)
                            throw new Exception("Cannot use member " + value + " for object of type " + obj.GetType());
                    }
                }

                var p = value as PropertyInfo;
                var f = value as FieldInfo;
                var type = p?.PropertyType ?? f?.FieldType;

                if (value != null)
                {
                    MemberName = value.Name;
                    MemberTypeName = type?.FullName ?? string.Empty;
                    ValueSize = GetValueDataSize(type);
                }
                else
                {
                    MemberName = string.Empty;
                    MemberTypeName = string.Empty;
                    ValueSize = 0;
                }

                OnMemberChanged(value, type);
            }
        }

        /// <summary>
        /// The preview value label.
        /// </summary>
        protected Label _previewValue;

        /// <summary>
        /// The navigate right keyframe icon.
        /// </summary>
        protected Image _rightKey;

        /// <summary>
        /// The add keyframe icon.
        /// </summary>
        protected Image _addKey;

        /// <summary>
        /// The navigate left keyframe icon.
        /// </summary>
        protected Image _leftKey;

        /// <summary>
        /// Initializes a new instance of the <see cref="MemberTrack"/> class.
        /// </summary>
        /// <param name="options">The track initial options.</param>
        /// <param name="useNavigationButtons">True if show keyframe navigation buttons, otherwise false.</param>
        /// <param name="useValuePreview">True if show current value preview, otherwise false.</param>
        /// <param name="useProxyKeyframes">True if show sub-tracks keyframes as a proxy on this track, otherwise false.</param>
        protected MemberTrack(ref TrackCreateOptions options, bool useNavigationButtons = true, bool useValuePreview = true, bool useProxyKeyframes = false)
        : base(ref options, useProxyKeyframes)
        {
            var uiLeft = _muteCheckbox.Offsets.Left;

            if (useNavigationButtons)
            {
                // Navigation buttons
                const float keySize = 18;
                const float addSize = 20;
                var icons = Editor.Instance.Icons;
                _rightKey = new Image
                {
                    TooltipText = "Sets the time to the next key",
                    AutoFocus = true,
                    AnchorPreset = AnchorPresets.MiddleRight,
                    IsScrollable = false,
                    Color = Style.Current.ForegroundGrey,
                    Margin = new Margin(1),
                    Brush = new SpriteBrush(icons.Right32),
                    Offsets = new Margin(-keySize - 2 + uiLeft, keySize, keySize * -0.5f, keySize),
                    Parent = this,
                };
                _addKey = new Image
                {
                    TooltipText = "Adds a new key at the current time",
                    AutoFocus = true,
                    AnchorPreset = AnchorPresets.MiddleRight,
                    IsScrollable = false,
                    Color = Style.Current.ForegroundGrey,
                    Margin = new Margin(3),
                    Brush = new SpriteBrush(icons.Add32),
                    Offsets = new Margin(-addSize - 2 + _rightKey.Offsets.Left, addSize, addSize * -0.5f, addSize),
                    Parent = this,
                };
                _leftKey = new Image
                {
                    TooltipText = "Sets the time to the previous key",
                    AutoFocus = true,
                    AnchorPreset = AnchorPresets.MiddleRight,
                    IsScrollable = false,
                    Color = Style.Current.ForegroundGrey,
                    Margin = new Margin(1),
                    Brush = new SpriteBrush(icons.Left32),
                    Offsets = new Margin(-keySize - 2 + _addKey.Offsets.Left, keySize, keySize * -0.5f, keySize),
                    Parent = this,
                };
                uiLeft = _leftKey.Offsets.Left;
            }

            if (useValuePreview)
            {
                // Value preview
                var previewWidth = 160.0f;
                _previewValueLeft = uiLeft;
                _previewValue = new Label
                {
                    AutoFocus = true,
                    AnchorPreset = AnchorPresets.MiddleRight,
                    Offsets = new Margin(-previewWidth - 2 + uiLeft, previewWidth, TextBox.DefaultHeight * -0.5f, TextBox.DefaultHeight),
                    IsScrollable = false,
                    AutoFitTextRange = new Float2(0.01f, 1.0f),
                    AutoFitText = true,
                    TextColor = Style.Current.ForegroundGrey,
                    Margin = new Margin(1),
                    HorizontalAlignment = TextAlignment.Far,
                    Parent = this
                };
            }
        }

        /// <summary>
        /// Evaluates the member value at the specified time.
        /// </summary>
        /// <param name="time">The time to evaluate the member at.</param>
        /// <returns>The member value at provided time.</returns>
        public virtual object Evaluate(float time)
        {
            return null;
        }

        /// <summary>
        /// Gets the member from the given type.
        /// </summary>
        /// <param name="type">The declaring type.</param>
        /// <param name="name">The member name.</param>
        /// <returns>The member or null if not found.</returns>
        protected MemberInfo GetMember(Type type, string name)
        {
            return type.GetMember(name, MemberTypes, BindingFlags.Public | BindingFlags.Instance).FirstOrDefault();
        }

        /// <summary>
        /// Gets the allowed member types for this track type.
        /// </summary>
        protected virtual MemberTypes MemberTypes => MemberTypes.Field | MemberTypes.Property;

        /// <summary>
        /// Tries the get current value from the assigned object property.
        /// </summary>
        /// <param name="value">The result value. Valid only if methods returns true.</param>
        /// <returns>True if got value, otherwise false.</returns>
        protected virtual bool TryGetValue(out object value)
        {
            if (!string.IsNullOrEmpty(MemberName) && ParentTrack is IObjectTrack objectTrack)
            {
                var obj = objectTrack.Object;
                if (obj != null)
                {
                    var member = GetMember(obj.GetType(), MemberName);
                    if (member is PropertyInfo p)
                    {
                        try
                        {
                            value = p.GetValue(obj);
                            return true;
                        }
                        catch
                        {
                            value = null;
                            return false;
                        }
                    }
                    if (member is FieldInfo f)
                    {
                        try
                        {
                            value = f.GetValue(obj);
                            return true;
                        }
                        catch
                        {
                            value = null;
                            return false;
                        }
                    }
                }
            }

            value = null;
            return false;
        }

        /// <summary>
        /// Gets the value text for UI.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The text.</returns>
        protected string GetValueText(object value)
        {
            if (value == null)
                return string.Empty;

            if (value is Quaternion asQuaternion)
                return asQuaternion.EulerAngles.ToString();

            return value.ToString();
        }

        /// <summary>
        /// Gets the size of the value data type.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The value data size (in bytes).</returns>
        protected virtual int GetValueDataSize(Type type)
        {
            return type != null && type.IsValueType ? (Marshal.SizeOf(type.IsEnum ? Enum.GetUnderlyingType(type) : type)) : 0;
        }

        /// <inheritdoc />
        public override bool CanDrag => false;

        /// <inheritdoc />
        public override bool CanRename => false;

        /// <inheritdoc />
        public override bool CanCopyPaste => false;

        /// <summary>
        /// Called when member gets changed.
        /// </summary>
        /// <param name="value">The member value assigned.</param>
        /// <param name="type">The member type assigned.</param>
        protected virtual void OnMemberChanged(MemberInfo value, Type type)
        {
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Timeline timeline)
        {
            if (Timeline != null && _previewValue != null)
            {
                Timeline.ShowPreviewValuesChanged -= OnTimelineShowPreviewValuesChanged;
            }

            base.OnTimelineChanged(timeline);

            if (Timeline != null && _previewValue != null)
            {
                _previewValue.Visible = Timeline.ShowPreviewValues;
                Timeline.ShowPreviewValuesChanged += OnTimelineShowPreviewValuesChanged;
            }
        }

        /// <inheritdoc />
        public override void OnDuplicated(Track clone)
        {
            base.OnDuplicated(clone);

            clone.Name = Guid.NewGuid().ToString("N");
        }

        private void OnTimelineShowPreviewValuesChanged()
        {
            _previewValue.Visible = Timeline.ShowPreviewValues;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            var p = Member;
            TitleTintColor = p != null ? Color.White : Color.Red;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            if (_previewValue != null)
            {
                // Based on Track.Draw for track text placement
                var left = _xOffset + 16 + Style.Current.FontSmall.MeasureText(Title ?? Name).X;
                if (Icon.IsValid)
                    left += 18;
                if (IsExpanded)
                    left = 2;

                // Limit preview value size to fit before the track text
                var previewWidth = Mathf.Min(200.0f, Width - left + _previewValueLeft - 12);
                _previewValue.Offsets = new Margin(-previewWidth - 2 + _previewValueLeft, previewWidth, TextBox.DefaultHeight * -0.5f, TextBox.DefaultHeight);
                _previewValue.Visible = Timeline.ShowPreviewValues && previewWidth > 10;
            }

            base.PerformLayoutBeforeChildren();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_previewValue != null)
                Timeline.ShowPreviewValuesChanged -= OnTimelineShowPreviewValuesChanged;
            _previewValue = null;
            _rightKey = null;
            _addKey = null;
            _leftKey = null;

            base.OnDestroy();
        }
    }
}
